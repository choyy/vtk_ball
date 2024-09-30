#include <array>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <filesystem>

#include <boost/program_options.hpp>

#include <vtkAutoInit.h>  
VTK_MODULE_INIT(vtkRenderingOpenGL2);  
VTK_MODULE_INIT(vtkInteractionStyle);  
  
#include <vtkSmartPointer.h>  
#include <vtkSphereSource.h>  
#include <vtkPolyDataMapper.h>  
#include <vtkActor.h>  
#include <vtkRenderer.h>  
#include <vtkRenderWindow.h>  
#include <vtkRenderWindowInteractor.h>  
#include <vtkProperty.h>  
#include <vtkLight.h>
#include <vtkCamera.h>  
#include <vtkAxesActor.h>
#include <vtkPNGWriter.h>  
#include <vtkWindowToImageFilter.h>

class vtkMyCallback : public vtkCommand {
public:
    static vtkMyCallback* New() { return new vtkMyCallback; }
    virtual void          Execute(vtkObject* caller, unsigned long, void*) {
        vtkRenderer* render = reinterpret_cast<vtkRenderer*>(caller);
        cout << render->GetActiveCamera()->GetPosition()[0] << " "
             << render->GetActiveCamera()->GetPosition()[1] << " "
             << render->GetActiveCamera()->GetPosition()[2] << endl;
    }
};

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
    // 定义命令行参数描述
    po::options_description desc("Allowed options");
    po::positional_options_description p;
    p.add("inputPose", -1);
    desc.add_options()
        ("help,h", "produce help message")
        ("inputPose", po::value<std::vector<std::string>>(), "camera pose parameters")
        ("focalLength,f", po::value<float>()->default_value(500), "set focal length")
        ("length,l", po::value<float>()->default_value(50.), "set distance of ball to origin")
        ("imageSize,s", po::value<std::string>()->default_value("1280x1024"), "set image size")
        ("showAxes,a", "show the axes")
        ("outputPath,o", po::value<std::string>()->default_value("img"), "set image output path");
    // 解析命令行参数
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    po::notify(vm);

    // 处理参数
    if (argc == 1 || vm.count("help")) {
        std::filesystem::path bin_name(argv[0]);
        std::cout << "Usage : " << "./" + bin_name.stem().string() << " z elevation azimuth [options]" << std::endl
                  << std::endl;
        std::cout << "z         : distance of camera to origin" << std::endl
                  << "elevation : camera elevation" << std::endl
                  << "azimuth   : camera azimuth" << std::endl
                  << "format    : start_value:step_size:end_value" << std::endl
                  << std::endl;
        std::cout << "[options]:" << std::endl
                  << "    -s, --imageSize    set image size, default " + vm["imageSize"].as<std::string>() << std::endl
                  << "    -f, --focalLength  set focal length, default " + std::to_string(vm["focalLength"].as<float>()) << std::endl
                  << "    -l, --length       set distance of ball to origin, default " + std::to_string(vm["length"].as<float>()) << std::endl
                  << "    -o, --outputPath   set image output path, default " + vm["outputPath"].as<std::string>() << std::endl
                  << "    -a, --showAxes     show the world axes" << std::endl;
        if (argc > 1) { return 0; }
    }
    std::vector<std::string> z, elevation, azimuth;
    if (vm.count("inputPose")) {
        const std::vector<std::string>& params = vm["inputPose"].as<std::vector<std::string>>();
        if (params.size() != 3) {
            std::cout << "Error: input camera pose must have 3 parameters" << std::endl;
            return 0;
        }
        std::stringstream ss(params[0]);
        std::string       str;
        while (getline(ss, str, ':')) { z.push_back(str); }
        ss.str(params[1]);
        ss.clear();
        while (getline(ss, str, ':')) { elevation.push_back(str); }
        ss.str(params[2]);
        ss.clear();
        while (getline(ss, str, ':')) { azimuth.push_back(str); }
        if (z.size() == 1) {
            z.push_back("1");
            z.push_back(std::to_string(std::stof(z[0]) + 1));
        }
        if (elevation.size() == 1) {
            elevation.push_back("1");
            elevation.push_back(std::to_string(std::stof(elevation[0]) + 1));
        }
        if (azimuth.size() == 1) {
            azimuth.push_back("1");
            azimuth.push_back(std::to_string(std::stof(azimuth[0]) + 1));
        }
    } else { // 默认参数
        z         = {"300", "1", "301"};
        elevation = {"45", "1", "46"};
        azimuth   = {"0", "1", "46"};
    }

    std::string imsize       = vm["imageSize"].as<std::string>();
    std::string output_path  = vm["outputPath"].as<std::string>();
    bool        if_show_axes = vm.count("showAxes") ? true : false;
    int         xnum         = imsize.find("x");

    // 相机内参 f 和图像尺寸 width, height
    int                   width           = std::stoi(imsize.substr(0, xnum));                 // 图像宽度
    int                   height          = std::stoi(imsize.substr(xnum + 1, imsize.size())); // 图像高度
    float                 f               = vm["focalLength"].as<float>();                     // 焦距
    float                 l               = vm["length"].as<float>();                          // 小球距离坐标原点的距离
    double                view_angle      = 2.0 * atan2(height / 2.0, f);                      // 垂直视场角
    std::array<double, 3> cam_pos         = {0, 0, -std::stof(z[0])};                          // 相机位置
    double                angle_elevation = std::stof(elevation[0]);                           // elevation 角度
    double                angle_azimuth   = std::stof(azimuth[0]);                             // azimuth 角度

    std::cout << "focal length: " << f << std::endl;
    std::cout << "distance of ball to origin: " << l << std::endl;
    std::cout << "image size: " << width << "x" << height << std::endl;
    std::cout << "output path: " << output_path << std::endl;
    std::cout << "show the axes: " << (if_show_axes ? "true" : "false") << std::endl;

    std::array<std::array<float, 7>, 7> spheres{
        //  x, y, z, r, g, b, radius
        0, 0, 0, 0, 1, 0, 20/2.,
        l, 0, 0, 1, 0, 0, 15/2.,
        0, l, 0, 0, 1, 0, 15/2.,
        0, 0, l, 0, 0, 1, 15/2.,
        -l, 0, 0,0, 1, 0, 15/2.,
        0, -l, 0,0, 0, 1, 15/2.,
        0, 0, -l,1, 0, 0, 15/2.,
    };
    std::vector<vtkSmartPointer<vtkSphereSource>> sphereSources;
    vtkSmartPointer<vtkRenderer>                  renderer = vtkSmartPointer<vtkRenderer>::New(); // 创建一个渲染器
    renderer->SetBackground(0, 0, 0);                                                             // 设置背景色
    for (int i = 0; i < 7; i++) {
        // 1. 创建球体源
        vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
        sphereSource->SetCenter(spheres[i][0], spheres[i][1], spheres[i][2]); // 设置球心
        sphereSource->SetRadius(spheres[i][6]);                               // 设置半径
        sphereSource->SetPhiResolution(50);                                   // 设置经度方向上的分辨率
        sphereSource->SetThetaResolution(50);                                 // 设置纬度方向上的分辨率
        sphereSources.push_back(sphereSource);

        // 2. 创建一个映射器，用于将球体数据映射到图形硬件
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(sphereSource->GetOutputPort());

        // 3. 创建一个actor，它是渲染场景中的一个实体
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(spheres[i][3], spheres[i][4], spheres[i][5]); // 设置颜色

        // 4. 添加actor到渲染器
        renderer->AddActor(actor);
    }
    if (if_show_axes) {
        // 创建坐标系对象
        vtkSmartPointer<vtkAxesActor> axes = vtkSmartPointer<vtkAxesActor>::New();
        axes->SetTotalLength(l + 50, l + 50, l + 50);     // 设置坐标轴的总长度
        axes->SetShaftType(vtkAxesActor::CYLINDER_SHAFT); // 设置坐标轴的类型为圆柱
        renderer->AddActor(axes);                         // 将坐标系添加到渲染器
    }

    // 获取并设置相机的参数(视场角)
    vtkCamera* camera = renderer->GetActiveCamera();
    camera->SetViewAngle(view_angle * 180.0 / vtkMath::Pi()); // 设置垂直视场角

    // // 创建光源
    // vtkLight* light = vtkLight::New();
    // light->SetFocalPoint(0, 0, 0);
    // light->SetPosition(1.2 * cam_pos[0], 1.2 * cam_pos[1], 1.2 * cam_pos[2]);
    // light->SetColor(1, 1, 1); // 设置光源颜色为白色
    // renderer->AddLight(light);

    // 创建一个渲染窗口
    vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);
    renderWindow->SetOffScreenRendering(1); // 开启OffScreen渲染
    renderWindow->SetSize(width, height);   // 设置窗口大小

    // vtkMyCallback* mo1 = vtkMyCallback::New();
    // renderer->AddObserver(vtkCommand::StartEvent, mo1);

    // 创建WindowToImageFilter
    vtkSmartPointer<vtkWindowToImageFilter> windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
    windowToImageFilter->SetInput(renderWindow);
    windowToImageFilter->Update();
    // 创建writer，这里以PNG格式为例
    vtkSmartPointer<vtkPNGWriter> writer = vtkSmartPointer<vtkPNGWriter>::New();
    writer->SetInputConnection(windowToImageFilter->GetOutputPort()); // 设置输入连接

    std::filesystem::path outpath(output_path); // 输出路径
    if (!std::filesystem::exists(outpath)) {    // 检查路径是否存在
        std::filesystem::create_directories(outpath);
    }
    std::ofstream pose_file(outpath / "pose.txt");
    for (float zi = std::stof(z[0]); zi < std::stof(z[2]); zi += std::stof(z[1]))
        for (float ei = std::stof(elevation[0]); ei < std::stof(elevation[2]); ei += std::stof(elevation[1]))
            for (float ai = std::stof(azimuth[0]); ai < std::stof(azimuth[2]); ai += std::stof(azimuth[1])) {
                camera->SetViewUp(0, -1, 0);        // 视上方向
                camera->SetPosition(0.0, 0.0, -zi); // 相机位置
                camera->Elevation(ei);              // 相机仰角
                camera->Azimuth(ai);                // 相机方位角
                renderWindow->Render();

                static int idx = 0;
                windowToImageFilter->Modified(); // 更新输出图片显示，如果不调用这个输出图片不会变化
                auto img_path = outpath / std::string(std::to_string(idx++) + ".png");
                std::cout << img_path.string() << " " << zi << " " << ei << " " << ai << std::endl;
                pose_file << img_path.string() << " " << zi << " " << ei << " " << ai << std::endl;
                writer->SetFileName(img_path.string().c_str()); // 输出文件名
                writer->Write();                                // 写入文件
            }
    std::cout << "images has been saved to " << (outpath / "*.png").string() << std::endl;
    std::cout << "pose has been saved to " << (outpath / "pose.txt").string() << std::endl;
    pose_file.close();

    // 创建一个渲染窗口交互器
    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    renderWindowInteractor->SetRenderWindow(renderWindow);
    renderWindowInteractor->Start(); // 开始交互

    return EXIT_SUCCESS;
}