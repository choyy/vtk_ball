#include <array>
#include <vector>


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
#include <vtkCamera.h>  
#include <vtkAxesActor.h>
#include <vtkPNGWriter.h>  
#include <vtkWindowToImageFilter.h>

class vtkMyCallback : public vtkCommand {
public:
    static vtkMyCallback* New()
    {
        return new vtkMyCallback;
    }
    virtual void Execute(vtkObject* caller, unsigned long, void*)
    {
        vtkRenderer* render = reinterpret_cast<vtkRenderer*>(caller);
        cout << render->GetActiveCamera()->GetPosition()[0] << " "
             << render->GetActiveCamera()->GetPosition()[1] << " "
             << render->GetActiveCamera()->GetPosition()[2] << endl;
    }
};

int main(int argc, char* argv[])
{
    // 相机内参 f 和图像尺寸 width, height
    int                   width      = 640;                          // 图像宽度
    int                   height     = 480;                          // 图像高度
    double                f          = 525.0;                        // 焦距
    double                view_angle = 2.0 * atan2(height / 2.0, f); // 垂直视场角
    std::array<double, 3> cam_pos    = {200, 0, 0};                  // 相机位置

    float l = 25.; // 小球距离坐标原点的距离
    std::array<std::array<float, 7>, 7> spheres {
    //  x, y, z, r, g, b, radius
        0, 0, 0, 1, 0, 0, 8,
        l, 0, 0, 0, 1, 0, 5,
        0, l, 0, 0, 1, 0, 5,
        0, 0, l, 0, 1, 0, 5,
        -l, 0, 0,1, 0, 0, 5,
        0, -l, 0,0, 0, 1, 5,
        0, 0, -l,0, 0, 1, 5,
    };
    std::vector<vtkSmartPointer<vtkSphereSource>> sphereSources;
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();  // 创建一个渲染器
    renderer->SetBackground(0, 0, 0); // 设置背景色
    for (int i = 0; i < 7; i++) {
        // 1. 创建球体源
        vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
        sphereSource->SetCenter         (spheres[i][0], spheres[i][1], spheres[i][2]);  // 设置球心
        sphereSource->SetRadius         (spheres[i][6]);                                // 设置半径
        sphereSource->SetPhiResolution  (50);                                           // 设置经度方向上的分辨率
        sphereSource->SetThetaResolution(50);                                           // 设置纬度方向上的分辨率
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
    // 创建坐标系对象
    vtkSmartPointer<vtkAxesActor> axes = vtkSmartPointer<vtkAxesActor>::New();
    axes->SetTotalLength(l + 30, l + 30, l + 30);     //设置坐标轴的总长度
    axes->SetShaftType(vtkAxesActor::CYLINDER_SHAFT); //设置坐标轴的类型为圆柱
    renderer->AddActor(axes);                         //将坐标系添加到渲染器

    // 获取并设置相机的位置和朝向  
    vtkCamera* camera = renderer->GetActiveCamera();
    camera->SetViewAngle(view_angle * 180.0 / vtkMath::Pi()); // 设置垂直视场角
    camera->SetViewUp(0, 0, 1);                               // 设置相机的上方向向量
    camera->SetPosition(cam_pos[0], cam_pos[1], cam_pos[2]);  // 设置相机位置

    // 创建一个渲染窗口
    vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);  
    renderWindow->SetSize(width, height); // 设置窗口大小

    // vtkMyCallback* mo1 = vtkMyCallback::New();
    // renderer->AddObserver(vtkCommand::StartEvent, mo1);

    // for (size_t i = 0; i < 360; i++) {
    //     renderWindow->Render();
    //     renderer->GetActiveCamera()->Azimuth(1);
	// }

    // 创建WindowToImageFilter
    vtkSmartPointer<vtkWindowToImageFilter> windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
    windowToImageFilter->SetInput(renderWindow);
    windowToImageFilter->ReadFrontBufferOff(); // 从后缓冲区读取，以避免闪烁
    windowToImageFilter->Update();
    // 创建writer，这里以PNG格式为例
    vtkSmartPointer<vtkPNGWriter> writer = vtkSmartPointer<vtkPNGWriter>::New();
    writer->SetFileName("output.png"); // 设置输出文件名
    writer->SetInputConnection(windowToImageFilter->GetOutputPort()); // 设置输入连接
    writer->Write(); // 写入文件

    // 创建一个渲染窗口交互器
    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    renderWindowInteractor->SetRenderWindow(renderWindow);  
  
    // 开始交互  
    renderWindow->Render();  
    renderWindowInteractor->Start();  
      
    return EXIT_SUCCESS;  
}