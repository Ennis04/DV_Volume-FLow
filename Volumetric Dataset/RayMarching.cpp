
// Modified from a modified version of the file described in the following comment...
//    -Chris

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: Medical1.cxx,v $
  Language:  C++
  Date:      $Date: 2002/11/27 16:06:38 $
  Version:   $Revision: 1.2 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVolume16Reader.h"
#include "vtkPolyDataMapper.h"
#include "vtkDataSetMapper.h"
#include "vtkActor.h"
#include "vtkOutlineFilter.h"
#include "vtkCamera.h"
#include "vtkProperty.h"
#include "vtkPolyDataNormals.h"
#include "vtkContourFilter.h"
#include "vtkStructuredPointsReader.h"
#include "vtkMarchingCubes.h"
#include "vtkRecursiveDividingCubes.h"
#include "vtkScalarBarWidget.h"
#include "vtkScalarBarActor.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkVolumeProperty.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkVolume.h"

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


  // vtkVolumeReader16 reads in the head CT data set.  
	vtkVolume16Reader *reader= vtkVolume16Reader::New();
    reader->SetDataDimensions (64,64);
    reader->SetImageRange (1,93);
    reader->SetDataByteOrderToLittleEndian();
	reader->SetFilePrefix("data/headsq/quarter");
    reader->SetDataSpacing (3.2, 3.2, 1.5);


  // This part creates the colorMap function to the volume rendering.
  vtkPiecewiseFunction* opacityTransferFunction = vtkPiecewiseFunction::New();
	opacityTransferFunction->AddPoint(20, 0.0);
	opacityTransferFunction->AddPoint(495, 0.0);
	opacityTransferFunction->AddPoint(500, 0.3);
	opacityTransferFunction->AddPoint(1150,0.5);
	opacityTransferFunction->AddPoint(1500, 0.9);

  vtkColorTransferFunction* colorTransferFunction = vtkColorTransferFunction::New();
	colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
	colorTransferFunction->AddRGBPoint(500.0, 1.0, 0.6, 0.0);
	colorTransferFunction->AddRGBPoint(700.0, 1.0, 0.6, 0.0);
	colorTransferFunction->AddRGBPoint(800.0, 1.0, 0.0, 0.0);
	colorTransferFunction->AddRGBPoint(1150.0, 0.9, 0.9, 0.9);
	
  // The property describes how the data will look
  vtkVolumeProperty* volumeProperty = vtkVolumeProperty::New();
	volumeProperty->SetColor(colorTransferFunction);
	volumeProperty->SetScalarOpacity(opacityTransferFunction);
	volumeProperty->ShadeOn();
	volumeProperty->SetInterpolationTypeToLinear();

  vtkGPUVolumeRayCastMapper* volumeMapper = vtkGPUVolumeRayCastMapper::New();
  volumeMapper->SetInputConnection(reader->GetOutputPort());
  volumeMapper->SetSampleDistance(1.0);

  // The volume holds the mapper and the property and
	// can be used to position/orient the volume
	vtkVolume* volume = vtkVolume::New();
	volume->SetMapper(volumeMapper);
	volume->SetProperty(volumeProperty);

 // An outline provides context around the data.
  vtkOutlineFilter *outlineData = vtkOutlineFilter::New();
    outlineData->SetInputConnection(reader->GetOutputPort());
  vtkPolyDataMapper *mapOutline = vtkPolyDataMapper::New();
    mapOutline->SetInputConnection(outlineData->GetOutputPort());
  vtkActor *outline = vtkActor::New();
    outline->SetMapper(mapOutline);
    outline->GetProperty()->SetColor(0,0,0);

  // It is convenient to create an initial view of the data. The FocalPoint
  // and Position form a vector direction. Later on (ResetCamera() method)
  // this vector is used to position the camera to look at the data in
  // this direction.
  vtkCamera *aCamera = vtkCamera::New();
    aCamera->SetViewUp (0, 0, -1);
    aCamera->SetPosition (0, 1, 0);
    aCamera->SetFocalPoint (0, 0, 0);
    aCamera->ComputeViewPlaneNormal();

  // Actors are added to the renderer. An initial camera view is created.
  // The Dolly() method moves the camera towards the FocalPoint,
  // thereby enlarging the image.
  aRenderer->AddActor(outline);
  aRenderer->AddVolume(volume);
  aRenderer->SetActiveCamera(aCamera);
  aRenderer->ResetCamera ();
  aCamera->Dolly(1.5);

  // Set a background color for the renderer and set the size of the
  // render window (expressed in pixels).
  aRenderer->SetBackground(1,1,1);
  renWin->SetSize(800, 600);

  // Note that when camera movement occurs (as it does in the Dolly()
  // method), the clipping planes often need adjusting. Clipping planes
  // consist of two planes: near and far along the view direction. The 
  // near plane clips out objects in front of the plane; the far plane
  // clips out objects behind the plane. This way only what is drawn
  // between the planes is actually rendered.
  aRenderer->ResetCameraClippingRange();


  vtkScalarBarWidget *scalarWidget = vtkScalarBarWidget::New();
  scalarWidget->SetInteractor(iren);
  scalarWidget->GetScalarBarActor()->SetTitle("Transfer Function");
  scalarWidget->GetScalarBarActor()->SetLookupTable(colorTransferFunction);


  // Initialize the event loop and then start it.
  iren->Initialize();
  renWin->SetWindowName( "Simple Volume Renderer" );
  renWin->Render();
  scalarWidget->EnabledOn();
  iren->Start(); 

  return 0;
}
