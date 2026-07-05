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

#include <iostream>
#include <string>


// Keyboard callback to switch rendering mode and change parameters.
class VolumeModeCallback : public vtkCommand
{
public:
    static VolumeModeCallback* New()
    {
        return new VolumeModeCallback;
    }

    vtkContourFilter* Contour = nullptr;
    vtkGPUVolumeRayCastMapper* VolumeMapper = nullptr;

    vtkActor* IsoActor1 = nullptr;
    vtkActor* IsoActor2 = nullptr;
    vtkVolume* RayVolume = nullptr;
    vtkScalarBarWidget* ScalarWidget = nullptr;
    vtkRenderWindow* RenderWindow = nullptr;

    bool IsIsoMode = true;

    double IsoValue = 500.0;
    double IsoStep = 50.0;
    double MinIsoValue = 100.0;
    double MaxIsoValue = 1500.0;

    double RayStep = 0.2;
    double MinSampleDistance = 0.1;
    double MaxSampleDistance = 10.0;

    void Execute(vtkObject* caller, unsigned long eventId, void* callData) override
    {
        vtkRenderWindowInteractor* interactor =
            static_cast<vtkRenderWindowInteractor*>(caller);

        std::string key = interactor->GetKeySym();

        // Switch to isosurface mode
        if (key == "1")
        {
            SetIsoMode(interactor);
            return;
        }

        // Switch to ray marching mode
        if (key == "2")
        {
            SetRayMode(interactor);
            return;
        }

        // Toggle mode
        if (key == "m" || key == "M")
        {
            if (IsIsoMode)
            {
                SetRayMode(interactor);
            }
            else
            {
                SetIsoMode(interactor);
            }
            return;
        }

        // Change iso-value or ray step size using + and -
        if (key == "plus" || key == "equal")
        {
            if (IsIsoMode)
            {
                IncreaseIsoValue(interactor);
            }
            else
            {
                IncreaseRayStep(interactor);
            }
            return;
        }

        if (key == "minus" || key == "underscore")
        {
            if (IsIsoMode)
            {
                DecreaseIsoValue(interactor);
            }
            else
            {
                DecreaseRayStep(interactor);
            }
            return;
        }
    }

    void SetIsoMode(vtkRenderWindowInteractor* interactor)
    {
        IsIsoMode = true;

        if (IsoActor1 != nullptr)
        {
            IsoActor1->SetVisibility(1);
        }

        if (IsoActor2 != nullptr)
        {
            IsoActor2->SetVisibility(1);
        }

        if (RayVolume != nullptr)
        {
            RayVolume->SetVisibility(0);
        }

        if (ScalarWidget != nullptr)
        {
            ScalarWidget->EnabledOff();
        }

        if (RenderWindow != nullptr)
        {
            RenderWindow->SetWindowName("Combined Volume Renderer - Isosurface Mode");
        }

        std::cout << "Mode: Isosurface" << std::endl;
        interactor->Render();
    }

    void SetRayMode(vtkRenderWindowInteractor* interactor)
    {
        IsIsoMode = false;

        if (IsoActor1 != nullptr)
        {
            IsoActor1->SetVisibility(0);
        }

        if (IsoActor2 != nullptr)
        {
            IsoActor2->SetVisibility(0);
        }

        if (RayVolume != nullptr)
        {
            RayVolume->SetVisibility(1);
        }

        if (ScalarWidget != nullptr)
        {
            ScalarWidget->EnabledOn();
        }

        if (RenderWindow != nullptr)
        {
            RenderWindow->SetWindowName("Combined Volume Renderer - Ray Marching Mode");
        }

        std::cout << "Mode: Ray Marching" << std::endl;
        interactor->Render();
    }

    void IncreaseIsoValue(vtkRenderWindowInteractor* interactor)
    {
        IsoValue += IsoStep;

        if (IsoValue > MaxIsoValue)
        {
            IsoValue = MaxIsoValue;
        }

        UpdateIsoValue(interactor);
    }

    void DecreaseIsoValue(vtkRenderWindowInteractor* interactor)
    {
        IsoValue -= IsoStep;

        if (IsoValue < MinIsoValue)
        {
            IsoValue = MinIsoValue;
        }

        UpdateIsoValue(interactor);
    }

    void UpdateIsoValue(vtkRenderWindowInteractor* interactor)
    {
        if (Contour == nullptr)
        {
            return;
        }

        Contour->SetValue(0, IsoValue);
        Contour->Modified();

        std::cout << "Current iso-value: " << IsoValue << std::endl;

        interactor->Render();
    }

    void IncreaseRayStep(vtkRenderWindowInteractor* interactor)
    {
        if (VolumeMapper == nullptr)
        {
            return;
        }

        double distance = VolumeMapper->GetSampleDistance();
        distance += RayStep;

        if (distance > MaxSampleDistance)
        {
            distance = MaxSampleDistance;
        }

        VolumeMapper->SetSampleDistance(distance);
        VolumeMapper->Modified();

        std::cout << "Current ray sample distance: " << distance << std::endl;

        interactor->Render();
    }

    void DecreaseRayStep(vtkRenderWindowInteractor* interactor)
    {
        if (VolumeMapper == nullptr)
        {
            return;
        }

        double distance = VolumeMapper->GetSampleDistance();
        distance -= RayStep;

        if (distance < MinSampleDistance)
        {
            distance = MinSampleDistance;
        }

        VolumeMapper->SetSampleDistance(distance);
        VolumeMapper->Modified();

        std::cout << "Current ray sample distance: " << distance << std::endl;

        interactor->Render();
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
	vtkVolume16Reader *reader= vtkVolume16Reader::New();
    reader->SetDataDimensions (64,64);
    reader->SetImageRange (1,93);
    reader->SetDataByteOrderToLittleEndian();
	reader->SetFilePrefix("data/headsq/quarter");
    reader->SetDataSpacing (3.2, 3.2, 1.5);


  // This next section creates two contours for the density data.  A
  //    vtkContourFilter object is created that takes the input data from
  //    the reader.																		
    vtkContourFilter *contourExtractor = vtkContourFilter::New();		
    contourExtractor->SetInputConnection( reader->GetOutputPort() ); 	
	contourExtractor->SetValue(0, 500);		

    vtkContourFilter *contourExtractor2 = vtkContourFilter::New();	
    contourExtractor2->SetInputConnection( reader->GetOutputPort() ); 											
	contourExtractor2->SetValue(0, 1150);
	

  // This section creates the polygon normals for the contour surfaces
  //    and creates the mapper that takes in the newly normalized surfaces
  vtkPolyDataNormals *contourNormals = vtkPolyDataNormals::New();
    contourNormals->SetInputConnection(contourExtractor->GetOutputPort());
    contourNormals->SetFeatureAngle(60.0);
  vtkPolyDataMapper *contourMapper = vtkPolyDataMapper::New();
    contourMapper->SetInputConnection(contourNormals->GetOutputPort());
    contourMapper->ScalarVisibilityOff();
  vtkPolyDataNormals *contourNormals2 = vtkPolyDataNormals::New();
    contourNormals2->SetInputConnection(contourExtractor2->GetOutputPort());
    contourNormals2->SetFeatureAngle(60.0);
  vtkPolyDataMapper *contourMapper2 = vtkPolyDataMapper::New();
    contourMapper2->SetInputConnection(contourNormals2->GetOutputPort());
    contourMapper2->ScalarVisibilityOff();


  // This section sets up the Actor that takes the contour
  //    This is where you can set the color and opacity of the two contours
  vtkActor *contour = vtkActor::New();
    contour->SetMapper(contourMapper);
	contour->GetProperty()->SetColor(0.8, 0.4, 0.0);
	contour->GetProperty()->SetOpacity(0.3);
  vtkActor *contour2 = vtkActor::New();
    contour2->SetMapper(contourMapper2);
	contour2->GetProperty()->SetColor(0.8, 0.8, 0.8);
	contour2->GetProperty()->SetOpacity(1.0);


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
  volumeMapper->AutoAdjustSampleDistancesOff();

  // The volume holds the mapper and the property and
	// can be used to position/orient the volume
	vtkVolume* volume = vtkVolume::New();
	volume->SetMapper(volumeMapper);
	volume->SetProperty(volumeProperty);
    volume->SetVisibility(0);
	

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
  aRenderer->AddActor(contour);
  aRenderer->AddActor(contour2);
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


  // Add keyboard callback to allow user to switch rendering modes and change parameters.
  VolumeModeCallback* modeCallback = VolumeModeCallback::New();
  modeCallback->Contour = contourExtractor;
  modeCallback->VolumeMapper = volumeMapper;
  modeCallback->IsoActor1 = contour;
  modeCallback->IsoActor2 = contour2;
  modeCallback->RayVolume = volume;
  modeCallback->ScalarWidget = scalarWidget;
  modeCallback->RenderWindow = renWin;
  modeCallback->IsIsoMode = true;
  modeCallback->IsoValue = 500.0;
  modeCallback->IsoStep = 50.0;
  modeCallback->MinIsoValue = 100.0;
  modeCallback->MaxIsoValue = 1500.0;
  modeCallback->RayStep = 0.2;

  iren->AddObserver(vtkCommand::KeyPressEvent, modeCallback);

  std::cout << "Combined volume renderer controls:" << std::endl;
  std::cout << "1 : switch to isosurface mode" << std::endl;
  std::cout << "2 : switch to ray marching mode" << std::endl;
  std::cout << "m : toggle between isosurface and ray marching" << std::endl;
  std::cout << "+ : increase iso-value or ray sample distance" << std::endl;
  std::cout << "- : decrease iso-value or ray sample distance" << std::endl;
  std::cout << "r : reset camera" << std::endl;
  std::cout << "q : quit" << std::endl;


  // Initialize the event loop and then start it.
  iren->Initialize();
  renWin->SetWindowName( "Combined Volume Renderer - Isosurface Mode" );
  renWin->Render();
  iren->Start(); 

  modeCallback->Delete();

  // It is important to delete all objects created previously to prevent
  // memory leaks. In this case, since the program is on its way to
  // exiting, it is not so important. But in applications it is
  // essential.
  reader->Delete();

  contourExtractor->Delete();
  contourNormals->Delete();
  contourMapper->Delete();
  contour->Delete();

  contourExtractor2->Delete();
  contourNormals2->Delete();
  contourMapper2->Delete();
  contour2->Delete();

  opacityTransferFunction->Delete();
  colorTransferFunction->Delete();
  volumeProperty->Delete();
  volumeMapper->Delete();
  volume->Delete();

  scalarWidget->Delete();

  outlineData->Delete();
  mapOutline->Delete();
  outline->Delete();
  aCamera->Delete();
  iren->Delete();
  renWin->Delete();
  aRenderer->Delete();

  return 0;
}