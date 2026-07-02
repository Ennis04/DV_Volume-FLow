#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStructuredPointsReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkLookupTable.h"
#include "vtkHedgeHog.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkPoints.h"

int main (int argc, char **argv)
{

  // Create the renderer, the render window, and the interactor. The renderer
  // draws into the render window, the interactor enables mouse- and 
  // keyboard-based interaction with the data within the render window.
	vtkRenderer *aRenderer = vtkRenderer::New();
	vtkRenderWindow *renWin = vtkRenderWindow::New();
	renWin->AddRenderer(aRenderer);
	vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
	iren->SetRenderWindow(renWin);

  // Read
	vtkStructuredPointsReader *reader= vtkStructuredPointsReader::New();
	reader->SetFileName("Flow Dataset/testData1.vtk");
	reader->Update();

  // Hedgehog setup.
	vtkHedgeHog *hhog = vtkHedgeHog::New();
	hhog->SetInputConnection( reader->GetOutputPort() );
	hhog->SetScaleFactor( 10 );

  // Lookup Table
	vtkLookupTable *lut = vtkLookupTable::New();
	lut->SetHueRange( 0.667, 0.0 );
	lut->Build();
 
  // Poly Mapper
	vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
	mapper->SetInputConnection( hhog->GetOutputPort() );
	mapper->SetScalarRange( 0.0, 1.0 );
	mapper->SetLookupTable( lut );

  // Actor
	vtkActor *actor = vtkActor::New();
    actor->SetMapper( mapper );

  // Actors are added to the renderer. An initial camera view is created.
  // The Dolly() method moves the camera towards the FocalPoint,
  // thereby enlarging the image.
	aRenderer->AddActor(actor);

  // Set a background color for the renderer and set the size of the
  // render window (expressed in pixels).
	aRenderer->SetBackground(0, 0, 0);
	renWin->SetSize(800, 600);

  // Initialize the event loop and then start it.
	iren->Initialize();
	renWin->SetWindowName( "Simple Volume Renderer" );
	renWin->Render();
	iren->Start(); 

	return 0;
}

