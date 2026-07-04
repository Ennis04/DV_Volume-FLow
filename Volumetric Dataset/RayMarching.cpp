
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
#include "vtkCommand.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include <iostream>
#include <string>

void SetupTransferFunction(std::string filePrefix, vtkColorTransferFunction* color, vtkPiecewiseFunction* opacity, double opacityShift) {
    color->RemoveAllPoints();
    opacity->RemoveAllPoints();

    if (filePrefix.find("foot") != std::string::npos) {
        // Foot dataset range is approx 0-255
        opacity->AddPoint(0 + opacityShift, 0.0);
        opacity->AddPoint(20 + opacityShift, 0.0);
        opacity->AddPoint(60 + opacityShift, 0.3); // Tissue
        opacity->AddPoint(120 + opacityShift, 0.6); // Bone
        opacity->AddPoint(255 + opacityShift, 0.9);

        color->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
        color->AddRGBPoint(60.0, 1.0, 0.6, 0.5); // Skin/Tissue colour
        color->AddRGBPoint(120.0, 0.9, 0.9, 0.9); // Bone colour
        color->AddRGBPoint(255.0, 1.0, 1.0, 1.0);
    } 
    else if (filePrefix.find("frog") != std::string::npos) {
        // Frog dataset
        opacity->AddPoint(0 + opacityShift, 0.0);
        opacity->AddPoint(10 + opacityShift, 0.0);
        opacity->AddPoint(40 + opacityShift, 0.2); // Outer layer
        opacity->AddPoint(80 + opacityShift, 0.5); // Inner tissues
        opacity->AddPoint(255 + opacityShift, 0.9);

        color->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
        color->AddRGBPoint(40.0, 0.2, 0.8, 0.2); // Greenish
        color->AddRGBPoint(80.0, 0.8, 0.2, 0.2); // Reddish internals
        color->AddRGBPoint(255.0, 0.9, 0.9, 0.9);
    } 
    else {
        // Default (Head dataset)
        opacity->AddPoint(20 + opacityShift, 0.0);
        opacity->AddPoint(495 + opacityShift, 0.0);
        opacity->AddPoint(500 + opacityShift, 0.3); // Skin
        opacity->AddPoint(1150 + opacityShift, 0.5); // Bone
        opacity->AddPoint(1500 + opacityShift, 0.9);

        color->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
        color->AddRGBPoint(500.0, 1.0, 0.6, 0.0);
        color->AddRGBPoint(700.0, 1.0, 0.6, 0.0);
        color->AddRGBPoint(800.0, 1.0, 0.0, 0.0);
        color->AddRGBPoint(1150.0, 0.9, 0.9, 0.9);
    }
}

class RayKeyInterpreter : public vtkCommand 
{
public:
    static RayKeyInterpreter *New() { return new RayKeyInterpreter; }
    
    vtkGPUVolumeRayCastMapper *map;
    vtkColorTransferFunction *ctf;
    vtkPiecewiseFunction *otf;
    std::string filePrefix;
    double currentOpacityShift;

    void Execute(vtkObject* caller, unsigned long eventId, void *callData) override
    {
        double dist;
        vtkRenderWindowInteractor *iren = reinterpret_cast<vtkRenderWindowInteractor *>(caller);
        switch(iren->GetKeyCode())
        {
            case '+':
            case '=':
                dist = map->GetSampleDistance();
                dist /= 1.5; // Decrease step size (increase quality)
                map->SetSampleDistance(dist);
                std::cout << "Ray step size decreased to: " << dist << std::endl;
                break;
            case '-':
            case '_':
                dist = map->GetSampleDistance();
                dist *= 1.5; // Increase step size (decrease quality)
                map->SetSampleDistance(dist);
                std::cout << "Ray step size increased to: " << dist << std::endl;
                break;
            case '[':
            case '{':
                currentOpacityShift -= 20.0;
                SetupTransferFunction(filePrefix, ctf, otf, currentOpacityShift);
                std::cout << "Opacity window shifted by: " << currentOpacityShift << std::endl;
                break;
            case ']':
            case '}':
                currentOpacityShift += 20.0;
                SetupTransferFunction(filePrefix, ctf, otf, currentOpacityShift);
                std::cout << "Opacity window shifted by: " << currentOpacityShift << std::endl;
                break;
        }
        iren->Render();
    }
};

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
  // Parse command line arguments for dataset loading
  std::string filePrefix = "data/headsq/quarter";
  int imageRange = 93;
  int dataDimX = 64, dataDimY = 64;
  double spacingX = 3.2, spacingY = 3.2, spacingZ = 1.5;

  if (argc > 1) {
      filePrefix = argv[1];
  }
  if (argc > 2) {
      imageRange = std::atoi(argv[2]);
  }
  if (argc > 4) {
      dataDimX = std::atoi(argv[3]);
      dataDimY = std::atoi(argv[4]);
  }
  if (argc > 7) {
      spacingX = std::atof(argv[5]);
      spacingY = std::atof(argv[6]);
      spacingZ = std::atof(argv[7]);
  }

	vtkVolume16Reader *reader= vtkVolume16Reader::New();
    reader->SetDataDimensions (dataDimX, dataDimY);
    reader->SetImageRange (1, imageRange);
    reader->SetDataByteOrderToLittleEndian();
	reader->SetFilePrefix(filePrefix.c_str());
    reader->SetDataSpacing (spacingX, spacingY, spacingZ);


  // This part creates the colorMap function to the volume rendering.
  vtkPiecewiseFunction* opacityTransferFunction = vtkPiecewiseFunction::New();
  vtkColorTransferFunction* colorTransferFunction = vtkColorTransferFunction::New();
  
  // Set up the initial dynamic transfer function based on the loaded dataset
  SetupTransferFunction(filePrefix, colorTransferFunction, opacityTransferFunction, 0.0);
	
  // The property describes how the data will look
  vtkVolumeProperty* volumeProperty = vtkVolumeProperty::New();
	volumeProperty->SetColor(colorTransferFunction);
	volumeProperty->SetScalarOpacity(opacityTransferFunction);
	volumeProperty->ShadeOn();
	volumeProperty->SetInterpolationTypeToLinear();

  vtkGPUVolumeRayCastMapper* volumeMapper = vtkGPUVolumeRayCastMapper::New();
  volumeMapper->SetInputConnection(reader->GetOutputPort());
  volumeMapper->SetAutoAdjustSampleDistances(0); // Disable auto-adjust so manual +/- changes are visible
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

  // Add on-screen text instructions
  vtkTextActor *txt = vtkTextActor::New();
  txt->SetInput("Controls:\n+/- : Change Ray Step Size\n[ / ] : Shift Opacity Window");
  vtkTextProperty *txtprop = txt->GetTextProperty();
  txtprop->SetFontSize(16);
  txtprop->SetColor(0.0, 0.0, 0.0); // Black text
  txt->SetDisplayPosition(10, 10);
  aRenderer->AddActor(txt);

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

  // To register the keyboard callback
  RayKeyInterpreter *key = RayKeyInterpreter::New();
  key->map = volumeMapper;
  key->ctf = colorTransferFunction;
  key->otf = opacityTransferFunction;
  key->filePrefix = filePrefix;
  key->currentOpacityShift = 0.0;
  iren->AddObserver(vtkCommand::KeyPressEvent, key);

  iren->Start(); 

  return 0;
}
