#include <iostream>
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkAutoInit.h" 
VTK_MODULE_INIT(vtkRenderingOpenGL2); 
VTK_MODULE_INIT(vtkInteractionStyle);
using namespace std;
class vtkMyCallback :public vtkCommand
{
public:
	static vtkMyCallback* New()
	{
		return new vtkMyCallback;
	}
	virtual void Execute(vtkObject *caller, unsigned long, void*)
	{
		vtkRenderer *render = reinterpret_cast<vtkRenderer*>(caller);
		cout << render->GetActiveCamera()->GetPosition()[0] << " "
			<< render->GetActiveCamera()->GetPosition()[1] << " "
			<< render->GetActiveCamera()->GetPosition()[2] << endl;
	}
};

int main()
{
	vtkConeSource *cone = vtkConeSource::New();
	cone->SetHeight(3.0);
	cone->SetRadius(1.0);
	cone->SetResolution(10);

	vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
	coneMapper->SetInputConnection(cone->GetOutputPort());
	vtkActor *coneActor = vtkActor::New();
	coneActor->SetMapper(coneMapper);

	vtkRenderer* ren1 = vtkRenderer::New();
	ren1->AddActor(coneActor);
	ren1->SetBackground(0.1, 0.2, 0.4);
	ren1->ResetCamera();

	vtkRenderWindow *renWin = vtkRenderWindow::New();
	renWin->AddRenderer(ren1);
	renWin->SetSize(300, 300);

	vtkMyCallback *mo1 = vtkMyCallback::New();
	ren1->AddObserver(vtkCommand::StartEvent, mo1);	
	mo1->Delete();

	for (size_t i = 0; i < 360; i++)
	{
		renWin->Render();
		ren1->GetActiveCamera()->Roll(1);
	}

	renWin->Delete();
	ren1->Delete();
	coneActor->Delete();
	coneMapper->Delete();
	cone->Delete();

	system("pause");
	return 0;
}
