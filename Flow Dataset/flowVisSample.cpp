#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStructuredPointsReader.h"
#include "vtkStructuredPoints.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkLookupTable.h"
#include "vtkHedgeHog.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkPoints.h"

#include "vtkCommand.h"
#include "vtkMaskPoints.h"
#include "vtkPolyData.h"
#include "vtkStreamTracer.h"
#include "vtkRungeKutta4.h"
#include "vtkTubeFilter.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkDataArray.h"
#include "vtkAssignAttribute.h"
#include "vtkDataSetAttributes.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkDataSet.h"
#include "vtkMath.h"

#include <iostream>
#include <string>
#include <algorithm>
#include <filesystem>
#include <cctype>
#include <vector>
#include <cmath>

namespace fs = std::filesystem;


// ------------------------------------------------------------
// Dataset configuration
// ------------------------------------------------------------
struct FlowDatasetConfig
{
    std::string name;
    std::string filePath;

    int dimX = 36;
    int dimY = 36;
    int dimZ = 1;

    bool is3D = false;

    int hedgeMaskRatio = 1;
    int glyphMaskRatio = 1;
    int streamGlyphMaskRatio = 5;

    double hedgehogScale = 1.0;
    double glyphScale = 1.0;
    double streamGlyphScale = 0.8;

    double coneHeight = 0.8;
    double coneRadius = 0.25;

    double tubeRadius = 0.08;

    int maxStreamSeeds = 80;

    double streamMaxPropagation = 100.0;
    double streamInitialStep = 0.5;
};


// ------------------------------------------------------------
// Helper functions
// ------------------------------------------------------------
std::string ToLower(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(),
        [](unsigned char c)
        {
            return std::tolower(c);
        });

    return text;
}


bool FileExists(const std::string& path)
{
    return fs::exists(fs::path(path));
}


std::string ResolveDatasetFile(const std::string& input)
{
    if (FileExists(input))
    {
        return input;
    }

    std::string lowerInput = ToLower(input);
    std::string name = input;

    if (lowerInput == "testdata1")
    {
        name = "testData1";
    }
    else if (lowerInput == "testdata2")
    {
        name = "testData2";
    }
    else if (lowerInput == "carotid")
    {
        name = "carotid";
    }

    std::string candidate1 = "Flow Dataset/" + name + ".vtk";
    std::string candidate2 = name + ".vtk";
    std::string candidate3 = "data/" + name + ".vtk";

    if (FileExists(candidate1))
    {
        return candidate1;
    }

    if (FileExists(candidate2))
    {
        return candidate2;
    }

    if (FileExists(candidate3))
    {
        return candidate3;
    }

    return "";
}


FlowDatasetConfig GetDatasetConfig(const std::string& input)
{
    FlowDatasetConfig config;

    std::string lowerInput = ToLower(input);
    std::string filePath = ResolveDatasetFile(input);

    config.filePath = filePath;

    if (lowerInput.find("testdata1") != std::string::npos)
    {
        config.name = "testData1";
        config.dimX = 36;
        config.dimY = 36;
        config.dimZ = 1;
        config.is3D = false;

        config.hedgeMaskRatio = 1;
        config.glyphMaskRatio = 1;
        config.streamGlyphMaskRatio = 3;

        config.hedgehogScale = 1.0;
        config.glyphScale = 1.0;
        config.streamGlyphScale = 0.8;

        config.coneHeight = 0.8;
        config.coneRadius = 0.25;
        config.tubeRadius = 0.06;

        config.maxStreamSeeds = 80;
        config.streamMaxPropagation = 100.0;
        config.streamInitialStep = 0.2;
    }
    else if (lowerInput.find("testdata2") != std::string::npos)
    {
        config.name = "testData2";
        config.dimX = 357;
        config.dimY = 357;
        config.dimZ = 1;
        config.is3D = false;

        config.hedgeMaskRatio = 20;
        config.glyphMaskRatio = 30;
        config.streamGlyphMaskRatio = 8;

        config.hedgehogScale = 0.15;
        config.glyphScale = 0.25;
        config.streamGlyphScale = 0.25;

        config.coneHeight = 1.0;
        config.coneRadius = 0.30;
        config.tubeRadius = 0.15;

        config.maxStreamSeeds = 100;
        config.streamMaxPropagation = 500.0;
        config.streamInitialStep = 1.0;
    }
    else if (lowerInput.find("carotid") != std::string::npos)
    {
        config.name = "carotid";
        config.dimX = 76;
        config.dimY = 49;
        config.dimZ = 45;
        config.is3D = true;

        config.hedgeMaskRatio = 80;
        config.glyphMaskRatio = 100;
        config.streamGlyphMaskRatio = 4;

        config.hedgehogScale = 1.0;
        config.glyphScale = 1.0;
        config.streamGlyphScale = 0.8;

        config.coneHeight = 0.8;
        config.coneRadius = 0.25;
        config.tubeRadius = 0.35;

        config.maxStreamSeeds = 120;
        config.streamMaxPropagation = 300.0;
        config.streamInitialStep = 0.5;
    }
    else
    {
        config.name = input;
        config.dimX = 50;
        config.dimY = 50;
        config.dimZ = 1;
        config.is3D = false;

        config.hedgeMaskRatio = 10;
        config.glyphMaskRatio = 10;
        config.streamGlyphMaskRatio = 5;

        config.hedgehogScale = 1.0;
        config.glyphScale = 1.0;
        config.streamGlyphScale = 0.8;

        config.coneHeight = 1.0;
        config.coneRadius = 0.30;
        config.tubeRadius = 0.1;

        config.maxStreamSeeds = 80;
        config.streamMaxPropagation = 100.0;
        config.streamInitialStep = 0.5;
    }

    return config;
}


// ------------------------------------------------------------
// Create streamline seed points from real dataset points.
// This avoids the carotid origin problem.
// ------------------------------------------------------------
vtkSmartPointer<vtkPolyData> CreateSeedPointsFromVectorMagnitude(
    vtkDataSet* dataSet,
    vtkDataArray* vectors,
    const FlowDatasetConfig& config)
{
    vtkSmartPointer<vtkPoints> points =
        vtkSmartPointer<vtkPoints>::New();

    if (dataSet == nullptr || vectors == nullptr)
    {
        vtkSmartPointer<vtkPolyData> emptySeeds =
            vtkSmartPointer<vtkPolyData>::New();
        emptySeeds->SetPoints(points);
        return emptySeeds;
    }

    vtkIdType numberOfPoints = dataSet->GetNumberOfPoints();

    std::vector<std::pair<double, vtkIdType>> candidates;

    for (vtkIdType i = 0; i < numberOfPoints; i++)
    {
        double v[3];
        vectors->GetTuple(i, v);

        double magnitude = std::sqrt(
            v[0] * v[0] +
            v[1] * v[1] +
            v[2] * v[2]
        );

        if (magnitude > 0.0000000001)
        {
            candidates.push_back(std::make_pair(magnitude, i));
        }
    }

    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const std::pair<double, vtkIdType>& a,
           const std::pair<double, vtkIdType>& b)
        {
            return a.first > b.first;
        }
    );

    std::cout << "Non-zero vector candidate points: "
              << candidates.size() << std::endl;

    if (candidates.empty())
    {
        vtkSmartPointer<vtkPolyData> emptySeeds =
            vtkSmartPointer<vtkPolyData>::New();
        emptySeeds->SetPoints(points);
        return emptySeeds;
    }

    int maxSeeds = config.maxStreamSeeds;

    int topLimit = static_cast<int>(candidates.size());

    if (topLimit > maxSeeds * 10)
    {
        topLimit = maxSeeds * 10;
    }

    int step = topLimit / maxSeeds;

    if (step < 1)
    {
        step = 1;
    }

    int inserted = 0;

    for (int i = 0; i < topLimit && inserted < maxSeeds; i += step)
    {
        vtkIdType pointId = candidates[i].second;

        double p[3];
        dataSet->GetPoint(pointId, p);

        points->InsertNextPoint(p);
        inserted++;
    }

    vtkSmartPointer<vtkPolyData> seedPolyData =
        vtkSmartPointer<vtkPolyData>::New();

    seedPolyData->SetPoints(points);

    double bounds[6];
    dataSet->GetBounds(bounds);

    std::cout << "Dataset bounds: "
              << "X[" << bounds[0] << ", " << bounds[1] << "] "
              << "Y[" << bounds[2] << ", " << bounds[3] << "] "
              << "Z[" << bounds[4] << ", " << bounds[5] << "]"
              << std::endl;

    std::cout << "Number of streamline seed points: "
              << points->GetNumberOfPoints() << std::endl;

    return seedPolyData;
}


// ------------------------------------------------------------
// Custom interactor style for switching visualisation modes
// ------------------------------------------------------------
class FlowInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
    static FlowInteractorStyle* New()
    {
        return new FlowInteractorStyle;
    }

    vtkActor* HedgehogActor = nullptr;
    vtkActor* GlyphActor = nullptr;
    vtkActor* StreamlineActor = nullptr;
    vtkActor* StreamGlyphActor = nullptr;

    vtkHedgeHog* HedgehogFilter = nullptr;
    vtkGlyph3D* GlyphFilter = nullptr;
    vtkGlyph3D* StreamGlyphFilter = nullptr;
    vtkTubeFilter* StreamTubeFilter = nullptr;

    vtkRenderer* Renderer = nullptr;
    vtkRenderWindow* RenderWindow = nullptr;

    int CurrentMode = 1;

    double HedgehogScale = 1.0;
    double GlyphScale = 1.0;
    double StreamGlyphScale = 1.0;
    double TubeRadius = 0.1;

    void OnChar() override
    {
        // Disable default VTK character shortcuts.
        // This prevents number keys from triggering stereo warnings.
    }

    void OnKeyPress() override
    {
        std::string key = this->Interactor->GetKeySym();

        if (key == "h" || key == "H")
        {
            SetMode(1);
            return;
        }

        if (key == "g" || key == "G")
        {
            SetMode(2);
            return;
        }

        if (key == "s" || key == "S")
        {
            SetMode(3);
            return;
        }

        if (key == "p" || key == "P")
        {
            SetMode(4);
            return;
        }

        if (key == "plus" || key == "equal")
        {
            IncreaseCurrentScale();
            return;
        }

        if (key == "minus" || key == "underscore")
        {
            DecreaseCurrentScale();
            return;
        }

        if (key == "r" || key == "R")
        {
            if (Renderer != nullptr)
            {
                Renderer->ResetCamera();
                Renderer->ResetCameraClippingRange();
            }

            if (this->Interactor != nullptr)
            {
                this->Interactor->Render();
            }

            return;
        }

        if (key == "q" || key == "Q" || key == "Escape")
        {
            if (this->Interactor != nullptr)
            {
                this->Interactor->TerminateApp();
            }

            return;
        }

        vtkInteractorStyleTrackballCamera::OnKeyPress();
    }

    void SetMode(int mode)
    {
        CurrentMode = mode;

        if (HedgehogActor != nullptr)
        {
            HedgehogActor->SetVisibility(mode == 1);
        }

        if (GlyphActor != nullptr)
        {
            GlyphActor->SetVisibility(mode == 2);
        }

        if (StreamlineActor != nullptr)
        {
            StreamlineActor->SetVisibility(mode == 3 || mode == 4);
        }

        if (StreamGlyphActor != nullptr)
        {
            StreamGlyphActor->SetVisibility(mode == 4);
        }

        if (Renderer != nullptr)
        {
            Renderer->ResetCamera();
            Renderer->ResetCameraClippingRange();
        }

        if (RenderWindow != nullptr)
        {
            if (mode == 1)
            {
                RenderWindow->SetWindowName("Flow Visualisation - Hedgehog Mode");
                std::cout << "Mode: Hedgehog" << std::endl;
            }
            else if (mode == 2)
            {
                RenderWindow->SetWindowName("Flow Visualisation - Cone Glyph Mode");
                std::cout << "Mode: Cone Glyph" << std::endl;
            }
            else if (mode == 3)
            {
                RenderWindow->SetWindowName("Flow Visualisation - Streamline Mode");
                std::cout << "Mode: Streamline" << std::endl;
            }
            else if (mode == 4)
            {
                RenderWindow->SetWindowName("Flow Visualisation - Streamline Glyph Mode");
                std::cout << "Mode: Streamline Glyph" << std::endl;
            }
        }

        if (this->Interactor != nullptr)
        {
            this->Interactor->Render();
        }
    }

    void IncreaseCurrentScale()
    {
        if (CurrentMode == 1 && HedgehogFilter != nullptr)
        {
            HedgehogScale *= 1.2;
            HedgehogFilter->SetScaleFactor(HedgehogScale);
            HedgehogFilter->Modified();
            std::cout << "Hedgehog scale: " << HedgehogScale << std::endl;
        }
        else if (CurrentMode == 2 && GlyphFilter != nullptr)
        {
            GlyphScale *= 1.2;
            GlyphFilter->SetScaleFactor(GlyphScale);
            GlyphFilter->Modified();
            std::cout << "Glyph scale: " << GlyphScale << std::endl;
        }
        else if (CurrentMode == 3 && StreamTubeFilter != nullptr)
        {
            TubeRadius *= 1.2;
            StreamTubeFilter->SetRadius(TubeRadius);
            StreamTubeFilter->Modified();
            std::cout << "Streamline tube radius: " << TubeRadius << std::endl;
        }
        else if (CurrentMode == 4 && StreamGlyphFilter != nullptr)
        {
            StreamGlyphScale *= 1.2;
            StreamGlyphFilter->SetScaleFactor(StreamGlyphScale);
            StreamGlyphFilter->Modified();
            std::cout << "Stream glyph scale: " << StreamGlyphScale << std::endl;
        }

        if (this->Interactor != nullptr)
        {
            this->Interactor->Render();
        }
    }

    void DecreaseCurrentScale()
    {
        if (CurrentMode == 1 && HedgehogFilter != nullptr)
        {
            HedgehogScale /= 1.2;
            HedgehogFilter->SetScaleFactor(HedgehogScale);
            HedgehogFilter->Modified();
            std::cout << "Hedgehog scale: " << HedgehogScale << std::endl;
        }
        else if (CurrentMode == 2 && GlyphFilter != nullptr)
        {
            GlyphScale /= 1.2;
            GlyphFilter->SetScaleFactor(GlyphScale);
            GlyphFilter->Modified();
            std::cout << "Glyph scale: " << GlyphScale << std::endl;
        }
        else if (CurrentMode == 3 && StreamTubeFilter != nullptr)
        {
            TubeRadius /= 1.2;
            StreamTubeFilter->SetRadius(TubeRadius);
            StreamTubeFilter->Modified();
            std::cout << "Streamline tube radius: " << TubeRadius << std::endl;
        }
        else if (CurrentMode == 4 && StreamGlyphFilter != nullptr)
        {
            StreamGlyphScale /= 1.2;
            StreamGlyphFilter->SetScaleFactor(StreamGlyphScale);
            StreamGlyphFilter->Modified();
            std::cout << "Stream glyph scale: " << StreamGlyphScale << std::endl;
        }

        if (this->Interactor != nullptr)
        {
            this->Interactor->Render();
        }
    }
};


int main(int argc, char** argv)
{
    std::string datasetInput = "testData1";

    if (argc >= 2)
    {
        datasetInput = argv[1];
    }

    FlowDatasetConfig config = GetDatasetConfig(datasetInput);

    if (config.filePath.empty())
    {
        std::cout << "Dataset file not found." << std::endl;
        std::cout << "Input: " << datasetInput << std::endl;
        std::cout << std::endl;
        std::cout << "Try one of these:" << std::endl;
        std::cout << "  FlowVis.exe testData1" << std::endl;
        std::cout << "  FlowVis.exe testData2" << std::endl;
        std::cout << "  FlowVis.exe carotid" << std::endl;
        std::cout << std::endl;
        std::cout << "Expected files:" << std::endl;
        std::cout << "  Flow Dataset/testData1.vtk" << std::endl;
        std::cout << "  Flow Dataset/testData2.vtk" << std::endl;
        std::cout << "  Flow Dataset/carotid.vtk" << std::endl;
        return 1;
    }

    std::cout << "Flow dataset selected: " << config.name << std::endl;
    std::cout << "File path: " << config.filePath << std::endl;
    std::cout << "Dataset size: "
              << config.dimX << " x "
              << config.dimY << " x "
              << config.dimZ << std::endl;


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
    reader->SetFileName(config.filePath.c_str());
    reader->Update();

    if (reader->GetOutput() == nullptr)
    {
        std::cout << "Error: unable to read dataset." << std::endl;
        return 1;
    }

    vtkPointData* pointData = reader->GetOutput()->GetPointData();

    if (pointData == nullptr)
    {
        std::cout << "Error: dataset has no point data." << std::endl;
        return 1;
    }

    std::cout << "Point data arrays:" << std::endl;

    std::string vectorArrayName = "";

    for (int i = 0; i < pointData->GetNumberOfArrays(); i++)
    {
        vtkDataArray* array = pointData->GetArray(i);

        if (array != nullptr)
        {
            const char* name = array->GetName();

            std::cout << "  Array " << i
                      << " | Name: " << (name ? name : "(no name)")
                      << " | Components: " << array->GetNumberOfComponents()
                      << std::endl;

            if (array->GetNumberOfComponents() == 3 && vectorArrayName.empty())
            {
                vectorArrayName = name ? name : "";
            }
        }
    }

    if (vectorArrayName.empty())
    {
        std::cout << "Error: No 3-component vector array found in this dataset." << std::endl;
        return 1;
    }

    std::cout << "Vector array selected: " << vectorArrayName << std::endl;


    // Force the 3-component array to be treated as vector data.
    vtkAssignAttribute* vectorSource = vtkAssignAttribute::New();
    vectorSource->SetInputConnection(reader->GetOutputPort());
    vectorSource->Assign(
        vectorArrayName.c_str(),
        vtkDataSetAttributes::VECTORS,
        vtkAssignAttribute::POINT_DATA
    );
    vectorSource->Update();

    vtkDataSet* vectorDataSet =
        vtkDataSet::SafeDownCast(vectorSource->GetOutputDataObject(0));

    if (vectorDataSet == nullptr)
    {
        std::cout << "Error: vectorSource output is not a vtkDataSet." << std::endl;
        return 1;
    }

    vtkDataArray* activeVectors =
        vectorDataSet->GetPointData()->GetVectors();

    if (activeVectors == nullptr)
    {
        std::cout << "Error: active vectors could not be assigned." << std::endl;
        return 1;
    }


  // Hedgehog setup.
    vtkMaskPoints* hedgeMask = vtkMaskPoints::New();
    hedgeMask->SetInputConnection(vectorSource->GetOutputPort());
    hedgeMask->SetOnRatio(config.hedgeMaskRatio);
    hedgeMask->RandomModeOff();

    vtkAssignAttribute* hedgeVectorSource = vtkAssignAttribute::New();
    hedgeVectorSource->SetInputConnection(hedgeMask->GetOutputPort());
    hedgeVectorSource->Assign(
        vectorArrayName.c_str(),
        vtkDataSetAttributes::VECTORS,
        vtkAssignAttribute::POINT_DATA
    );

    vtkHedgeHog *hhog = vtkHedgeHog::New();
    hhog->SetInputConnection(hedgeVectorSource->GetOutputPort());
    hhog->SetScaleFactor(config.hedgehogScale);


  // Glyph setup.
    vtkMaskPoints* glyphMask = vtkMaskPoints::New();
    glyphMask->SetInputConnection(vectorSource->GetOutputPort());
    glyphMask->SetOnRatio(config.glyphMaskRatio);
    glyphMask->RandomModeOff();

    vtkAssignAttribute* glyphVectorSource = vtkAssignAttribute::New();
    glyphVectorSource->SetInputConnection(glyphMask->GetOutputPort());
    glyphVectorSource->Assign(
        vectorArrayName.c_str(),
        vtkDataSetAttributes::VECTORS,
        vtkAssignAttribute::POINT_DATA
    );

    vtkConeSource* cone = vtkConeSource::New();
    cone->SetResolution(16);
    cone->SetHeight(config.coneHeight);
    cone->SetRadius(config.coneRadius);
    cone->SetDirection(1.0, 0.0, 0.0);

    vtkGlyph3D* glyph = vtkGlyph3D::New();
    glyph->SetInputConnection(glyphVectorSource->GetOutputPort());
    glyph->SetSourceConnection(cone->GetOutputPort());
    glyph->SetScaleFactor(config.glyphScale);
    glyph->SetScaleModeToScaleByVector();
    glyph->SetVectorModeToUseVector();
    glyph->OrientOn();


  // Streamline setup.
    vtkSmartPointer<vtkPolyData> seedPolyData =
        CreateSeedPointsFromVectorMagnitude(vectorDataSet, activeVectors, config);

    vtkRungeKutta4* integrator = vtkRungeKutta4::New();

    vtkStreamTracer* streamTracer = vtkStreamTracer::New();
    streamTracer->SetInputConnection(vectorSource->GetOutputPort());
    streamTracer->SetSourceData(seedPolyData);
    streamTracer->SetIntegrator(integrator);
    streamTracer->SetMaximumPropagation(config.streamMaxPropagation);
    streamTracer->SetInitialIntegrationStep(config.streamInitialStep);
    streamTracer->SetMinimumIntegrationStep(0.01);
    streamTracer->SetMaximumIntegrationStep(5.0);
    streamTracer->SetMaximumNumberOfSteps(5000);
    streamTracer->SetTerminalSpeed(0.000000000001);
    streamTracer->SetIntegrationDirectionToBoth();
    streamTracer->SetComputeVorticity(false);
    streamTracer->Update();

    std::cout << "Streamline output points: "
              << streamTracer->GetOutput()->GetNumberOfPoints()
              << std::endl;

    std::cout << "Streamline output lines: "
              << streamTracer->GetOutput()->GetNumberOfLines()
              << std::endl;

    vtkTubeFilter* streamTube = vtkTubeFilter::New();
    streamTube->SetInputConnection(streamTracer->GetOutputPort());
    streamTube->SetRadius(config.tubeRadius);
    streamTube->SetNumberOfSides(8);


  // Streamline glyph setup.
  // This places cone glyphs along the generated streamline points.
    vtkMaskPoints* streamPointMask = vtkMaskPoints::New();
    streamPointMask->SetInputConnection(streamTracer->GetOutputPort());
    streamPointMask->SetOnRatio(config.streamGlyphMaskRatio);
    streamPointMask->RandomModeOff();

    vtkAssignAttribute* streamVectorSource = vtkAssignAttribute::New();
    streamVectorSource->SetInputConnection(streamPointMask->GetOutputPort());
    streamVectorSource->Assign(
        vectorArrayName.c_str(),
        vtkDataSetAttributes::VECTORS,
        vtkAssignAttribute::POINT_DATA
    );

    vtkConeSource* streamCone = vtkConeSource::New();
    streamCone->SetResolution(16);
    streamCone->SetHeight(config.coneHeight);
    streamCone->SetRadius(config.coneRadius);
    streamCone->SetDirection(1.0, 0.0, 0.0);

    vtkGlyph3D* streamGlyph = vtkGlyph3D::New();
    streamGlyph->SetInputConnection(streamVectorSource->GetOutputPort());
    streamGlyph->SetSourceConnection(streamCone->GetOutputPort());
    streamGlyph->SetScaleFactor(config.streamGlyphScale);
    streamGlyph->SetScaleModeToDataScalingOff();
    streamGlyph->SetVectorModeToUseVector();
    streamGlyph->OrientOn();


  // Lookup Table
    vtkLookupTable *lut = vtkLookupTable::New();
    lut->SetHueRange(0.667, 0.0);
    lut->Build();


  // Poly Mapper
    vtkPolyDataMapper *hedgeMapper = vtkPolyDataMapper::New();
    hedgeMapper->SetInputConnection(hhog->GetOutputPort());
    hedgeMapper->SetLookupTable(lut);
    hedgeMapper->ScalarVisibilityOff();

    vtkPolyDataMapper* glyphMapper = vtkPolyDataMapper::New();
    glyphMapper->SetInputConnection(glyph->GetOutputPort());
    glyphMapper->SetLookupTable(lut);
    glyphMapper->ScalarVisibilityOff();

    vtkPolyDataMapper* streamMapper = vtkPolyDataMapper::New();
    streamMapper->SetInputConnection(streamTube->GetOutputPort());
    streamMapper->SetLookupTable(lut);
    streamMapper->ScalarVisibilityOff();

    vtkPolyDataMapper* streamGlyphMapper = vtkPolyDataMapper::New();
    streamGlyphMapper->SetInputConnection(streamGlyph->GetOutputPort());
    streamGlyphMapper->SetLookupTable(lut);
    streamGlyphMapper->ScalarVisibilityOff();


  // Actor
    vtkActor *hedgeActor = vtkActor::New();
    hedgeActor->SetMapper(hedgeMapper);
    hedgeActor->GetProperty()->SetColor(0.1, 1.0, 0.2);

    vtkActor* glyphActor = vtkActor::New();
    glyphActor->SetMapper(glyphMapper);
    glyphActor->GetProperty()->SetColor(1.0, 0.6, 0.1);

    vtkActor* streamActor = vtkActor::New();
    streamActor->SetMapper(streamMapper);
    streamActor->GetProperty()->SetColor(0.2, 0.7, 1.0);

    vtkActor* streamGlyphActor = vtkActor::New();
    streamGlyphActor->SetMapper(streamGlyphMapper);
    streamGlyphActor->GetProperty()->SetColor(1.0, 1.0, 0.2);


  // Actors are added to the renderer. An initial camera view is created.
  // The Dolly() method moves the camera towards the FocalPoint,
  // thereby enlarging the image.
    aRenderer->AddActor(hedgeActor);
    aRenderer->AddActor(glyphActor);
    aRenderer->AddActor(streamActor);
    aRenderer->AddActor(streamGlyphActor);

    hedgeActor->SetVisibility(1);
    glyphActor->SetVisibility(0);
    streamActor->SetVisibility(0);
    streamGlyphActor->SetVisibility(0);

    aRenderer->ResetCamera();

    vtkCamera* camera = aRenderer->GetActiveCamera();

    if (config.is3D)
    {
        camera->Azimuth(35);
        camera->Elevation(25);
    }

    camera->Dolly(1.3);
    aRenderer->ResetCameraClippingRange();


  // Set a background color for the renderer and set the size of the
  // render window (expressed in pixels).
    aRenderer->SetBackground(0, 0, 0);
    renWin->SetSize(900, 700);


    FlowInteractorStyle* style = FlowInteractorStyle::New();

    style->SetDefaultRenderer(aRenderer);
    style->SetCurrentRenderer(aRenderer);

    style->Renderer = aRenderer;
    style->RenderWindow = renWin;

    style->HedgehogActor = hedgeActor;
    style->GlyphActor = glyphActor;
    style->StreamlineActor = streamActor;
    style->StreamGlyphActor = streamGlyphActor;

    style->HedgehogFilter = hhog;
    style->GlyphFilter = glyph;
    style->StreamGlyphFilter = streamGlyph;
    style->StreamTubeFilter = streamTube;

    style->CurrentMode = 1;

    style->HedgehogScale = config.hedgehogScale;
    style->GlyphScale = config.glyphScale;
    style->StreamGlyphScale = config.streamGlyphScale;
    style->TubeRadius = config.tubeRadius;

    iren->SetInteractorStyle(style);


    std::cout << std::endl;
    std::cout << "Flow visualisation controls:" << std::endl;
    std::cout << "h : Hedgehog mode" << std::endl;
    std::cout << "g : Cone glyph mode" << std::endl;
    std::cout << "s : Streamline mode" << std::endl;
    std::cout << "p : Streamline glyph mode" << std::endl;
    std::cout << "+ : increase current mode scale" << std::endl;
    std::cout << "- : decrease current mode scale" << std::endl;
    std::cout << "r : reset camera" << std::endl;
    std::cout << "q : quit" << std::endl;
    std::cout << std::endl;

    std::cout << "Initial parameters:" << std::endl;
    std::cout << "Hedgehog scale factor: " << config.hedgehogScale << std::endl;
    std::cout << "Glyph scale factor: " << config.glyphScale << std::endl;
    std::cout << "Streamline tube radius: " << config.tubeRadius << std::endl;
    std::cout << "Stream glyph scale factor: " << config.streamGlyphScale << std::endl;
    std::cout << std::endl;


  // Initialize the event loop and then start it.
    iren->Initialize();
    renWin->SetWindowName("Flow Visualisation - Hedgehog Mode");
    renWin->Render();
    iren->Start();


    style->Delete();

    vectorSource->Delete();
    hedgeVectorSource->Delete();
    glyphVectorSource->Delete();
    streamVectorSource->Delete();

    reader->Delete();

    hedgeMask->Delete();
    hhog->Delete();

    glyphMask->Delete();
    cone->Delete();
    glyph->Delete();

    integrator->Delete();
    streamTracer->Delete();
    streamTube->Delete();

    streamPointMask->Delete();
    streamCone->Delete();
    streamGlyph->Delete();

    lut->Delete();

    hedgeMapper->Delete();
    glyphMapper->Delete();
    streamMapper->Delete();
    streamGlyphMapper->Delete();

    hedgeActor->Delete();
    glyphActor->Delete();
    streamActor->Delete();
    streamGlyphActor->Delete();

    iren->Delete();
    renWin->Delete();
    aRenderer->Delete();

    return 0;
}