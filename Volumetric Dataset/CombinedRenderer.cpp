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

class CombinedKeyInterpreter : public vtkCommand 
{
public:
    static CombinedKeyInterpreter* New() { return new CombinedKeyInterpreter; }

    vtkContourFilter* Contour1 = nullptr;
    vtkContourFilter* Contour2 = nullptr;
    vtkGPUVolumeRayCastMapper* VolumeMapper = nullptr;
    vtkColorTransferFunction *Ctf = nullptr;
    vtkPiecewiseFunction *Otf = nullptr;

    vtkActor* IsoActor1 = nullptr;
    vtkActor* IsoActor2 = nullptr;
    vtkVolume* RayVolume = nullptr;
    vtkScalarBarWidget* ScalarWidget = nullptr;
    vtkRenderWindow* RenderWindow = nullptr;
    vtkTextActor* TextInstructions = nullptr;

    bool IsIsoMode = true;
    double IsoValue = 500.0;
    std::string FilePrefix;
    double CurrentOpacityShift = 0.0;

    void UpdateText()
    {
        if (!TextInstructions) return;
        char buffer[256];
        if (IsIsoMode) {
            snprintf(buffer, sizeof(buffer), "Controls (Isosurface Mode):\n1 : Isosurface Mode\n2 : Ray Marching Mode\nm : Toggle Modes\n+/- : Change Iso-value\n\nCurrent Iso-Value: %.1f", IsoValue);
        } else {
            double dist = VolumeMapper ? VolumeMapper->GetSampleDistance() : 1.0;
            snprintf(buffer, sizeof(buffer), "Controls (Ray Marching Mode):\n1 : Isosurface Mode\n2 : Ray Marching Mode\nm : Toggle Modes\n+/- : Change Ray Step Size\n[ / ] : Shift Opacity\n\nCurrent Ray Step: %.3f\nOpacity Shift: %.1f", dist, CurrentOpacityShift);
        }
        TextInstructions->SetInput(buffer);
    }

    void Execute(vtkObject* caller, unsigned long eventId, void* callData) override
    {
        vtkRenderWindowInteractor* interactor = static_cast<vtkRenderWindowInteractor*>(caller);
        std::string key = interactor->GetKeySym();
        char keyCode = interactor->GetKeyCode(); 

        if (key == "1") {
            SetIsoMode(interactor);
            return;
        }
        if (key == "2") {
            SetRayMode(interactor);
            return;
        }
        if (key == "m" || key == "M") {
            if (IsIsoMode) SetRayMode(interactor);
            else SetIsoMode(interactor);
            return;
        }

        if (keyCode == '+' || keyCode == '=') {
            if (IsIsoMode) {
                IsoValue += 50.0;
                if (Contour1) Contour1->SetValue(0, IsoValue);
                std::cout << "Iso-value increased to: " << IsoValue << std::endl;
            } else {
                if (VolumeMapper) {
                    double dist = VolumeMapper->GetSampleDistance() / 1.5;
                    VolumeMapper->SetSampleDistance(dist);
                    std::cout << "Ray step size decreased to: " << dist << std::endl;
                }
            }
            UpdateText();
            interactor->Render();
            return;
        }

        if (keyCode == '-' || keyCode == '_') {
            if (IsIsoMode) {
                IsoValue -= 50.0;
                if (IsoValue < 0.0) IsoValue = 0.0;
                if (Contour1) Contour1->SetValue(0, IsoValue);
                std::cout << "Iso-value decreased to: " << IsoValue << std::endl;
            } else {
                if (VolumeMapper) {
                    double dist = VolumeMapper->GetSampleDistance() * 1.5;
                    VolumeMapper->SetSampleDistance(dist);
                    std::cout << "Ray step size increased to: " << dist << std::endl;
                }
            }
            UpdateText();
            interactor->Render();
            return;
        }

        if (!IsIsoMode) {
            if (keyCode == '[' || keyCode == '{') {
                CurrentOpacityShift -= 20.0;
                SetupTransferFunction(FilePrefix, Ctf, Otf, CurrentOpacityShift);
                std::cout << "Opacity window shifted by: " << CurrentOpacityShift << std::endl;
                interactor->Render();
            }
            else if (keyCode == ']' || keyCode == '}') {
                CurrentOpacityShift += 20.0;
                SetupTransferFunction(FilePrefix, Ctf, Otf, CurrentOpacityShift);
                std::cout << "Opacity window shifted by: " << CurrentOpacityShift << std::endl;
            }
        }
        UpdateText();
        interactor->Render();
    }

    void SetIsoMode(vtkRenderWindowInteractor* interactor)
    {
        IsIsoMode = true;
        if (IsoActor1) IsoActor1->SetVisibility(1);
        if (IsoActor2) IsoActor2->SetVisibility(1);
        if (RayVolume) RayVolume->SetVisibility(0);
        if (ScalarWidget) ScalarWidget->EnabledOff();
        
        if (RenderWindow) RenderWindow->SetWindowName("Combined Volume Renderer - Isosurface Mode");
        UpdateText();
        
        std::cout << "Mode: Isosurface" << std::endl;
        interactor->Render();
    }

    void SetRayMode(vtkRenderWindowInteractor* interactor)
    {
        IsIsoMode = false;
        if (IsoActor1) IsoActor1->SetVisibility(0);
        if (IsoActor2) IsoActor2->SetVisibility(0);
        if (RayVolume) RayVolume->SetVisibility(1);
        if (ScalarWidget) ScalarWidget->EnabledOn();
        
        if (RenderWindow) RenderWindow->SetWindowName("Combined Volume Renderer - Ray Marching Mode");
        UpdateText();
        
        std::cout << "Mode: Ray Marching" << std::endl;
        interactor->Render();
    }
};

int main (int argc, char **argv)
{
	vtkRenderer *aRenderer = vtkRenderer::New();
	vtkRenderWindow *renWin = vtkRenderWindow::New();
	renWin->AddRenderer(aRenderer);
	vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
	iren->SetRenderWindow(renWin);

  std::string filePrefix = "data/headsq/quarter";
  int imageRange = 93;
  int dataDimX = 64, dataDimY = 64;
  double spacingX = 3.2, spacingY = 3.2, spacingZ = 1.5;

  if (argc > 1) { filePrefix = argv[1]; }
  if (argc > 2) { imageRange = std::atoi(argv[2]); }
  if (argc > 4) { dataDimX = std::atoi(argv[3]); dataDimY = std::atoi(argv[4]); }
  if (argc > 7) { spacingX = std::atof(argv[5]); spacingY = std::atof(argv[6]); spacingZ = std::atof(argv[7]); }

	vtkVolume16Reader *reader= vtkVolume16Reader::New();
    reader->SetDataDimensions (dataDimX, dataDimY);
    reader->SetImageRange (1, imageRange);
    reader->SetDataByteOrderToLittleEndian();
	reader->SetFilePrefix(filePrefix.c_str());
    reader->SetDataSpacing (spacingX, spacingY, spacingZ);

  double initialIso1 = 500.0;
  double initialIso2 = 1150.0;
  if (filePrefix.find("foot") != std::string::npos) {
      initialIso1 = 60.0;
      initialIso2 = 120.0;
  } else if (filePrefix.find("frog") != std::string::npos) {
      initialIso1 = 40.0;
      initialIso2 = 80.0;
  }

  // --- ISOSURFACE SETUP ---
    vtkContourFilter *contourExtractor = vtkContourFilter::New();		
    contourExtractor->SetInputConnection( reader->GetOutputPort() ); 	
	contourExtractor->SetValue(0, initialIso1);		

    vtkContourFilter *contourExtractor2 = vtkContourFilter::New();	
    contourExtractor2->SetInputConnection( reader->GetOutputPort() ); 											
	contourExtractor2->SetValue(0, initialIso2);
	
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

  vtkActor *contour = vtkActor::New();
    contour->SetMapper(contourMapper);
	contour->GetProperty()->SetColor(0.8, 0.4, 0.0);
	contour->GetProperty()->SetOpacity(0.3);
  vtkActor *contour2 = vtkActor::New();
    contour2->SetMapper(contourMapper2);
	contour2->GetProperty()->SetColor(0.8, 0.8, 0.8);
	contour2->GetProperty()->SetOpacity(1.0);

  // --- RAY MARCHING SETUP ---
  vtkPiecewiseFunction* opacityTransferFunction = vtkPiecewiseFunction::New();
  vtkColorTransferFunction* colorTransferFunction = vtkColorTransferFunction::New();
  SetupTransferFunction(filePrefix, colorTransferFunction, opacityTransferFunction, 0.0);
	
  vtkVolumeProperty* volumeProperty = vtkVolumeProperty::New();
	volumeProperty->SetColor(colorTransferFunction);
	volumeProperty->SetScalarOpacity(opacityTransferFunction);
	volumeProperty->ShadeOn();
	volumeProperty->SetInterpolationTypeToLinear();

  vtkGPUVolumeRayCastMapper* volumeMapper = vtkGPUVolumeRayCastMapper::New();
  volumeMapper->SetInputConnection(reader->GetOutputPort());
  volumeMapper->SetAutoAdjustSampleDistances(0); 
  volumeMapper->SetSampleDistance(1.0);

	vtkVolume* volume = vtkVolume::New();
	volume->SetMapper(volumeMapper);
	volume->SetProperty(volumeProperty);

  // --- GENERAL SCENE SETUP ---
  vtkOutlineFilter *outlineData = vtkOutlineFilter::New();
    outlineData->SetInputConnection(reader->GetOutputPort());
  vtkPolyDataMapper *mapOutline = vtkPolyDataMapper::New();
    mapOutline->SetInputConnection(outlineData->GetOutputPort());
  vtkActor *outline = vtkActor::New();
    outline->SetMapper(mapOutline);
    outline->GetProperty()->SetColor(0,0,0);

  vtkCamera *aCamera = vtkCamera::New();
    aCamera->SetViewUp (0, 0, -1);
    aCamera->SetPosition (0, 1, 0);
    aCamera->SetFocalPoint (0, 0, 0);
    aCamera->ComputeViewPlaneNormal();

  aRenderer->AddActor(outline);
  aRenderer->AddActor(contour);
  aRenderer->AddActor(contour2);
  aRenderer->AddVolume(volume);

  vtkTextActor *txt = vtkTextActor::New();
  vtkTextProperty *txtprop = txt->GetTextProperty();
  txtprop->SetFontSize(16);
  txtprop->SetColor(0.0, 0.0, 0.0); 
  txt->SetDisplayPosition(10, 10);
  aRenderer->AddActor(txt);

  aRenderer->SetActiveCamera(aCamera);
  aRenderer->ResetCamera ();
  aCamera->Dolly(1.5);

  aRenderer->SetBackground(1,1,1);
  renWin->SetSize(800, 600);
  aRenderer->ResetCameraClippingRange();

  vtkScalarBarWidget *scalarWidget = vtkScalarBarWidget::New();
  scalarWidget->SetInteractor(iren);
  scalarWidget->GetScalarBarActor()->SetTitle("Transfer Function");
  scalarWidget->GetScalarBarActor()->SetLookupTable(colorTransferFunction);

  // --- INITIALIZE & START ---
  iren->Initialize();

  CombinedKeyInterpreter *key = CombinedKeyInterpreter::New();
  key->Contour1 = contourExtractor;
  key->Contour2 = contourExtractor2;
  key->VolumeMapper = volumeMapper;
  key->Ctf = colorTransferFunction;
  key->Otf = opacityTransferFunction;
  key->IsoActor1 = contour;
  key->IsoActor2 = contour2;
  key->RayVolume = volume;
  key->ScalarWidget = scalarWidget;
  key->RenderWindow = renWin;
  key->TextInstructions = txt;
  key->FilePrefix = filePrefix;
  key->IsoValue = initialIso1;
  key->CurrentOpacityShift = 0.0;
  
  iren->AddObserver(vtkCommand::KeyPressEvent, key);
  
  // Start in Isosurface mode
  key->SetIsoMode(iren);

  iren->Start(); 

  // Cleanup
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
  txt->Delete();
  key->Delete();
  iren->Delete();
  renWin->Delete();
  aRenderer->Delete();

  return 0;
}
