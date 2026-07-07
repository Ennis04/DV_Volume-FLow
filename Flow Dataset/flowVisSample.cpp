#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStructuredPointsReader.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridReader.h"
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
#include "vtkThresholdPoints.h"
#include "vtkAlgorithmOutput.h"
#include "vtkAlgorithm.h"
#include "vtkPolyData.h"
#include "vtkStreamTracer.h"
#include "vtkRungeKutta4.h"
#include "vtkTubeFilter.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkAssignAttribute.h"
#include "vtkDataSetAttributes.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkDataSet.h"
#include "vtkMath.h"

#include <iostream>
#include <fstream>
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


// ------------------------------------------------------------
// Peek at the first few lines of a legacy VTK file to determine its
// DATASET type. testData1/testData2/carotid are all STRUCTURED_POINTS
// (uniform grid, read with vtkStructuredPointsReader), but other
// engineering datasets - like the classic kitchen.vtk airflow example
// - are STRUCTURED_GRID (curvilinear/non-uniform grid, needs
// vtkStructuredGridReader instead). This lets main() pick the right
// reader automatically without the user having to specify it.
// ------------------------------------------------------------
bool IsStructuredGridFile(const std::string& path)
{
    std::ifstream file(path);

    if (!file.is_open())
    {
        return false;
    }

    std::string line;
    int linesChecked = 0;

    while (std::getline(file, line) && linesChecked < 10)
    {
        if (line.find("STRUCTURED_GRID") != std::string::npos)
        {
            return true;
        }

        if (line.find("STRUCTURED_POINTS") != std::string::npos)
        {
            return false;
        }

        linesChecked++;
    }

    return false;
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
    else if (lowerInput == "kitchen")
    {
        name = "kitchen";
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

        config.hedgeMaskRatio = 1;
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
    else if (lowerInput.find("kitchen") != std::string::npos)
    {
        // Classic VTK-textbook airflow-in-a-kitchen dataset. It's a
        // STRUCTURED_GRID (non-uniform/curvilinear), unlike the other
        // three STRUCTURED_POINTS datasets - main() auto-detects this
        // from the file itself and swaps readers accordingly. dimX/Y/Z
        // below are placeholders; they get overwritten with the real
        // dimensions read from the file right after loading.
        config.name = "kitchen";
        config.dimX = 28;
        config.dimY = 24;
        config.dimZ = 13;
        config.is3D = true;

        config.hedgeMaskRatio = 1;
        config.glyphMaskRatio = 1;
        config.streamGlyphMaskRatio = 4;

        config.hedgehogScale = 1.0;
        config.glyphScale = 1.0;
        config.streamGlyphScale = 0.8;

        config.coneHeight = 0.5;
        config.coneRadius = 0.2;
        config.tubeRadius = 0.05;

        config.maxStreamSeeds = 100;
        config.streamMaxPropagation = 100.0;
        config.streamInitialStep = 0.2;
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
// Create streamline seed points on a regular grid, spanning the
// exact index ranges given in the assignment:
//   testData1: 0 <= x, y <= 36,  z = 0
//   testData2: 0 <= x, y <= 357, z = 0
//   carotid:   0 <= x <= 76, 0 <= y <= 49, 0 <= z <= 45
//
// These are GRID-INDEX ranges (they match config.dimX/dimY/dimZ
// exactly). Rather than computing world coordinates from an ORIGIN +
// SPACING (which only exists for uniform-grid datasets), each chosen
// grid index is converted to world space with dataSet->GetPoint(id),
// which works correctly for BOTH a uniform STRUCTURED_POINTS grid
// (carotid's non-zero ORIGIN is handled automatically) and a
// non-uniform/curvilinear STRUCTURED_GRID (e.g. kitchen.vtk), since
// vtkDataSet::GetPoint() returns the real coordinate for either type.
//
// For 3D datasets an additional magnitude filter is applied: e.g.
// 85%+ of the carotid volume is near-static background tissue, not
// vessel flow. Seeding there just produces long, chaotic, meaningless
// streamlines as RK4 wanders through near-zero noise. So candidate
// grid points below noiseFloor are skipped entirely - the grid is
// still scanned across the full specified index range, it just only
// keeps the points that are inside the region VTK's own data says
// actually has meaningful flow.
// ------------------------------------------------------------
vtkSmartPointer<vtkPolyData> CreateRegularGridSeedPoints(
    vtkDataSet* dataSet,
    vtkDataArray* magnitudeArray,
    double noiseFloor,
    const FlowDatasetConfig& config)
{
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    int maxIndexX = config.dimX;
    int maxIndexY = config.dimY;
    int maxIndexZ = config.dimZ;

    if (dataSet == nullptr)
    {
        vtkSmartPointer<vtkPolyData> emptySeeds = vtkSmartPointer<vtkPolyData>::New();
        emptySeeds->SetPoints(points);
        return emptySeeds;
    }

    if (config.is3D && magnitudeArray != nullptr)
    {
        // Scan every grid vertex across the full specified index
        // range, but only keep the ones with real flow (magnitude
        // above the noise floor). Point ordering matches the raw
        // structured layout: x fastest, then y, then z.
        std::vector<vtkIdType> qualifyingIds;

        for (int k = 0; k < maxIndexZ; k++)
        {
            for (int j = 0; j < maxIndexY; j++)
            {
                for (int i = 0; i < maxIndexX; i++)
                {
                    vtkIdType pointId =
                        static_cast<vtkIdType>(k) * maxIndexY * maxIndexX
                        + static_cast<vtkIdType>(j) * maxIndexX
                        + i;

                    double mag = magnitudeArray->GetTuple1(pointId);

                    if (mag >= noiseFloor)
                    {
                        qualifyingIds.push_back(pointId);
                    }
                }
            }
        }

        std::cout << "Seed candidates above noise floor: "
                  << qualifyingIds.size() << " (of "
                  << (maxIndexX * maxIndexY * maxIndexZ) << ")" << std::endl;

        if (!qualifyingIds.empty())
        {
            // Sort candidates by magnitude, strongest first. Picking
            // the top-N strongest points (rather than an even stride
            // through everything that merely clears the noise floor)
            // avoids seeding in weak, borderline/turbulent pixels that
            // only produce short chaotic little loops - it concentrates
            // seeds in the clearest, most coherent flow.
            std::sort(
                qualifyingIds.begin(),
                qualifyingIds.end(),
                [magnitudeArray](vtkIdType a, vtkIdType b)
                {
                    return magnitudeArray->GetTuple1(a) > magnitudeArray->GetTuple1(b);
                }
            );

            int seedCount = config.maxStreamSeeds;

            if (seedCount > static_cast<int>(qualifyingIds.size()))
            {
                seedCount = static_cast<int>(qualifyingIds.size());
            }

            for (int idx = 0; idx < seedCount; idx++)
            {
                vtkIdType pointId = qualifyingIds[static_cast<size_t>(idx)];

                double coords[3];
                dataSet->GetPoint(pointId, coords);

                points->InsertNextPoint(coords);
            }
        }
    }
    else
    {
        // Flat (z = 0) 2D datasets, or 3D datasets with no magnitude
        // filter available: simple even regular grid across the full
        // specified index range.
        int samplesPerAxis = static_cast<int>(
            std::round(std::sqrt(static_cast<double>(config.maxStreamSeeds))));

        if (samplesPerAxis < 2)
        {
            samplesPerAxis = 2;
        }

        int samplesX = samplesPerAxis;
        int samplesY = samplesPerAxis;

        for (int yi = 0; yi < samplesY; yi++)
        {
            int indexY = (samplesY > 1)
                ? static_cast<int>(std::round((static_cast<double>(yi) / (samplesY - 1)) * (maxIndexY - 1)))
                : 0;

            for (int xi = 0; xi < samplesX; xi++)
            {
                int indexX = (samplesX > 1)
                    ? static_cast<int>(std::round((static_cast<double>(xi) / (samplesX - 1)) * (maxIndexX - 1)))
                    : 0;

                vtkIdType pointId =
                    static_cast<vtkIdType>(indexY) * maxIndexX + indexX;

                double coords[3];
                dataSet->GetPoint(pointId, coords);

                points->InsertNextPoint(coords);
            }
        }
    }

    vtkSmartPointer<vtkPolyData> seedPolyData =
        vtkSmartPointer<vtkPolyData>::New();

    seedPolyData->SetPoints(points);

    std::cout << "Regular-grid streamline seed points: "
              << points->GetNumberOfPoints() << std::endl;

    return seedPolyData;
}


// ------------------------------------------------------------
// (Legacy) Create streamline seed points from real dataset points.
// Kept for reference - CreateRegularGridSeedPoints() above is what
// the assignment actually asks for (an explicit index-range grid),
// so this is no longer called from main().
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

        // Mode switching is now done with the number keys 1-4
        // (previously h / g / s / p).
        if (key == "1")
        {
            SetMode(1);
            return;
        }

        if (key == "2")
        {
            SetMode(2);
            return;
        }

        if (key == "3")
        {
            SetMode(3);
            return;
        }

        if (key == "4")
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
        std::cout << "  FlowVis.exe kitchen" << std::endl;
        std::cout << std::endl;
        std::cout << "Expected files:" << std::endl;
        std::cout << "  Flow Dataset/testData1.vtk" << std::endl;
        std::cout << "  Flow Dataset/testData2.vtk" << std::endl;
        std::cout << "  Flow Dataset/carotid.vtk" << std::endl;
        std::cout << "  Flow Dataset/kitchen.vtk" << std::endl;
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
  // Auto-detect whether this is a STRUCTURED_POINTS file (testData1,
  // testData2, carotid - uniform grid) or a STRUCTURED_GRID file
  // (e.g. kitchen.vtk - curvilinear/non-uniform grid) and use the
  // matching reader. Everything downstream works generically off
  // vtkAlgorithm/vtkDataSet, so the rest of the pipeline doesn't need
  // to know which one was used.
    bool isStructuredGrid = IsStructuredGridFile(config.filePath);

    vtkSmartPointer<vtkStructuredPointsReader> pointsReader;
    vtkSmartPointer<vtkStructuredGridReader> gridReader;

    vtkAlgorithm* readerAlgorithm = nullptr;
    vtkDataSet* readerOutputDataSet = nullptr;

    if (isStructuredGrid)
    {
        std::cout << "Detected STRUCTURED_GRID file - using vtkStructuredGridReader." << std::endl;

        gridReader = vtkSmartPointer<vtkStructuredGridReader>::New();
        gridReader->SetFileName(config.filePath.c_str());
        gridReader->Update();

        readerAlgorithm = gridReader;
        readerOutputDataSet = gridReader->GetOutput();
    }
    else
    {
        pointsReader = vtkSmartPointer<vtkStructuredPointsReader>::New();
        pointsReader->SetFileName(config.filePath.c_str());
        pointsReader->Update();

        readerAlgorithm = pointsReader;
        readerOutputDataSet = pointsReader->GetOutput();
    }

    if (readerOutputDataSet == nullptr)
    {
        std::cout << "Error: unable to read dataset." << std::endl;
        return 1;
    }

    vtkPointData* pointData = readerOutputDataSet->GetPointData();

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


    // ------------------------------------------------------------
    // Detect the REAL dimensions/grid spacing from the file itself,
    // overriding config.dimX/dimY/dimZ/is3D from GetDatasetConfig()'s
    // hardcoded guesses. This matters most for new datasets (e.g.
    // kitchen.vtk) where we don't want to have to get the exact
    // numbers right by hand, and it must happen BEFORE the magnitude
    // percentile calculations below, since those depend on config.is3D.
    // ------------------------------------------------------------
    double gridSpacing = 1.0;

    vtkStructuredPoints* structuredPointsData =
        vtkStructuredPoints::SafeDownCast(readerOutputDataSet);

    vtkStructuredGrid* structuredGridData =
        vtkStructuredGrid::SafeDownCast(readerOutputDataSet);

    if (structuredPointsData != nullptr)
    {
        double spacing[3];
        int dims[3];

        structuredPointsData->GetSpacing(spacing);
        structuredPointsData->GetDimensions(dims);

        config.dimX = dims[0];
        config.dimY = dims[1];
        config.dimZ = dims[2];
        config.is3D = (dims[2] > 1);

        double spacingSum = 0.0;
        int spacingCount = 0;

        for (int d = 0; d < 3; d++)
        {
            // Only count dimensions that actually vary (skip the flat
            // Z axis on 2D datasets like testData1 / testData2).
            if (dims[d] > 1)
            {
                spacingSum += spacing[d];
                spacingCount++;
            }
        }

        if (spacingCount > 0)
        {
            gridSpacing = spacingSum / spacingCount;
        }
    }
    else if (structuredGridData != nullptr)
    {
        // Non-uniform/curvilinear grid (e.g. kitchen.vtk): there's no
        // single SPACING value, so approximate an average cell size
        // from the bounding box divided by the number of cells along
        // each active axis.
        int dims[3];
        structuredGridData->GetDimensions(dims);

        config.dimX = dims[0];
        config.dimY = dims[1];
        config.dimZ = dims[2];
        config.is3D = (dims[2] > 1);

        double bounds[6];
        structuredGridData->GetBounds(bounds);

        double spacingSum = 0.0;
        int spacingCount = 0;

        for (int d = 0; d < 3; d++)
        {
            if (dims[d] > 1)
            {
                double extent = bounds[2 * d + 1] - bounds[2 * d];
                spacingSum += extent / (dims[d] - 1);
                spacingCount++;
            }
        }

        if (spacingCount > 0)
        {
            gridSpacing = spacingSum / spacingCount;
        }

        std::cout << "Structured grid dimensions (auto-detected): "
                  << config.dimX << " x " << config.dimY << " x " << config.dimZ
                  << std::endl;
    }


    // ------------------------------------------------------------
    // Build a scalar "VectorMagnitude" array from the chosen vector
    // field. This is what drives the colour scale below - without a
    // scalar array attached to the point data, ScalarVisibilityOn()
    // has nothing to colour by, which is why everything was rendering
    // in a single flat colour before.
    // ------------------------------------------------------------
    vtkDataArray* rawVectorArray = pointData->GetArray(vectorArrayName.c_str());

    vtkSmartPointer<vtkDoubleArray> magnitudeArray =
        vtkSmartPointer<vtkDoubleArray>::New();
    magnitudeArray->SetName("VectorMagnitude");
    magnitudeArray->SetNumberOfComponents(1);

    vtkIdType numMagPoints = rawVectorArray->GetNumberOfTuples();
    magnitudeArray->SetNumberOfTuples(numMagPoints);

    double magMin = VTK_DOUBLE_MAX;
    double magMax = -VTK_DOUBLE_MAX;

    std::vector<double> magSamples(static_cast<size_t>(numMagPoints));

    for (vtkIdType i = 0; i < numMagPoints; i++)
    {
        double v[3];
        rawVectorArray->GetTuple(i, v);

        double mag = vtkMath::Norm(v);
        magnitudeArray->SetTuple1(i, mag);
        magSamples[static_cast<size_t>(i)] = mag;

        if (mag < magMin)
        {
            magMin = mag;
        }

        if (mag > magMax)
        {
            magMax = mag;
        }
    }

    // Guard against a degenerate (flat) range.
    if (magMax <= magMin)
    {
        magMax = magMin + 1.0;
    }

    // ------------------------------------------------------------
    // Use the 95th-percentile magnitude (not the mean) as the
    // reference for scaling. Some datasets (carotid in particular)
    // have a huge dynamic range - a small jet-core region can be
    // 100-1000x faster than the rest of the field - so the mean gets
    // dragged way up by a handful of points and produces absurdly
    // long hedgehog lines / cones everywhere else. The 95th
    // percentile is far more representative of "typical fast flow"
    // and ignores the few most extreme outliers.
    // ------------------------------------------------------------
    std::vector<double> sortedMag = magSamples;
    std::sort(sortedMag.begin(), sortedMag.end());

    auto percentileOf = [&sortedMag](double fraction) -> double
    {
        size_t idx = static_cast<size_t>(fraction * (sortedMag.size() - 1) + 0.5);

        if (idx >= sortedMag.size())
        {
            idx = sortedMag.size() - 1;
        }

        return sortedMag[idx];
    };

    // For 3D datasets (carotid), carotid's magnitude distribution has
    // a huge natural gap around the 97th-98th percentile (background
    // tissue jumps straight to real vessel-flow speeds there) - use
    // that as the display cap so the filtered "real flow" points
    // still show meaningful length/size variation between each
    // other. 2D datasets keep the original 95th-percentile cap.
    double displayCap = config.is3D ? percentileOf(0.995) : percentileOf(0.95);

    if (displayCap <= 0.0)
    {
        displayCap = (magMax > 0.0) ? magMax : 1.0;
    }

    // ------------------------------------------------------------
    // Noise floor: some 3D datasets (carotid) have a sharp natural
    // gap in their magnitude distribution - background tissue sits
    // far below it, real vessel flow sits far above it. Other 3D
    // datasets (kitchen's gentle room convection) have a smooth,
    // continuous distribution with no such gap - forcing a fixed
    // percentile cutoff onto them just chops off most of the
    // legitimate flow. So instead of a fixed percentile, scan the
    // upper half of the sorted magnitude distribution for the
    // single BIGGEST proportional jump between nearby samples. If
    // that jump is large (>= 5x), there's a genuine background/signal
    // split and we filter right at that gap. If nothing jumps that
    // much, the distribution is smooth and no filtering is applied
    // (2D datasets always skip this entirely).
    // ------------------------------------------------------------
    double noiseFloor = magMin;
    bool backgroundGapFound = false;

    if (config.is3D)
    {
        int sampleCount = static_cast<int>(sortedMag.size());
        int startIdx = static_cast<int>(0.50 * (sampleCount - 1));
        int endIdx = static_cast<int>(0.995 * (sampleCount - 1));
        int step = std::max(1, (endIdx - startIdx) / 200);

        double bestRatio = 1.0;
        int bestIdx = -1;

        for (int idx = startIdx; idx + step <= endIdx; idx += step)
        {
            double lower = sortedMag[idx];
            double upper = sortedMag[idx + step];

            if (lower > 1e-12)
            {
                double ratio = upper / lower;

                if (ratio > bestRatio)
                {
                    bestRatio = ratio;
                    bestIdx = idx + step;
                }
            }
        }

        const double gapRatioThreshold = 5.0;

        if (bestIdx >= 0 && bestRatio >= gapRatioThreshold)
        {
            noiseFloor = sortedMag[bestIdx];
            backgroundGapFound = true;

            std::cout << "Detected a natural background/signal gap ("
                      << bestRatio << "x jump) - noise floor set to: "
                      << noiseFloor << std::endl;
        }
        else
        {
            std::cout << "No strong background/signal gap detected "
                      << "(largest jump was " << bestRatio
                      << "x) - no noise-floor filtering applied." << std::endl;
        }
    }

    pointData->AddArray(magnitudeArray);
    pointData->SetActiveScalars("VectorMagnitude");

    // ------------------------------------------------------------
    // Build a magnitude-CLIPPED copy of the vector field, used only
    // for the hedgehog line length and cone-glyph scaling. Any vector
    // longer than displayCap is rescaled down to displayCap while
    // keeping its direction, guaranteeing a bounded render size no
    // matter how extreme a handful of outlier vectors are.
    // The true (unclipped) field is still used for the streamline
    // integration and for the colour legend, so the fast-flow region
    // is still correctly identified by colour even though its glyph
    // size saturates.
    // ------------------------------------------------------------
    vtkSmartPointer<vtkDoubleArray> vectorDisplayArray =
        vtkSmartPointer<vtkDoubleArray>::New();
    vectorDisplayArray->SetName("VectorDisplay");
    vectorDisplayArray->SetNumberOfComponents(3);
    vectorDisplayArray->SetNumberOfTuples(numMagPoints);

    for (vtkIdType i = 0; i < numMagPoints; i++)
    {
        double v[3];
        rawVectorArray->GetTuple(i, v);

        double mag = magSamples[static_cast<size_t>(i)];

        if (mag > displayCap && mag > 0.0)
        {
            double factor = displayCap / mag;
            v[0] *= factor;
            v[1] *= factor;
            v[2] *= factor;
        }

        vectorDisplayArray->SetTuple(i, v);
    }

    pointData->AddArray(vectorDisplayArray);

    std::cout << "Vector magnitude range: ["
              << magMin << ", " << magMax << "]" << std::endl;
    std::cout << "Vector magnitude display cap (95th pct, or 99.5th for 3D): "
              << displayCap << std::endl;
    std::cout << "Vector magnitude noise floor (97th percentile, 3D only): "
              << noiseFloor << std::endl;


    // (dims/gridSpacing detection moved earlier - see above, right
    // after vector array selection - so config.is3D is accurate
    // before the magnitude-percentile calculations that depend on it)


    // CLIPPED vector field. Since VectorDisplay is capped at
    // displayCap, the longest possible line is now guaranteed to be
    // (0.9 * gridSpacing) regardless of how extreme the raw data is.
    double autoHedgehogScale = (0.9 * gridSpacing) / displayCap;

    // Cone glyph: rendered height = coneHeight * glyphScale * magnitude
    // (ScaleModeToScaleByVector), also driven by the clipped field for
    // the same bounded-size guarantee.
    double autoGlyphScale = (0.9 * gridSpacing) / (config.coneHeight * displayCap);

    // Streamline tube radius: keep it a small fraction of spacing so
    // tubes don't fuse into a solid tunnel.
    double autoTubeRadius = 0.12 * gridSpacing;

    // Stream glyph cones: ScaleModeToDataScalingOff, so rendered size
    // = coneHeight * streamGlyphScale (no magnitude dependence). Make
    // these a bit larger than the tube so they read as arrowheads.
    double autoStreamGlyphScale = (1.6 * gridSpacing) / config.coneHeight;

    config.hedgehogScale = autoHedgehogScale;
    config.glyphScale = autoGlyphScale;
    config.tubeRadius = autoTubeRadius;
    config.streamGlyphScale = autoStreamGlyphScale;

    std::cout << "Grid spacing (auto-detected): " << gridSpacing << std::endl;
    std::cout << "Auto-tuned hedgehog scale: " << config.hedgehogScale << std::endl;
    std::cout << "Auto-tuned glyph scale: " << config.glyphScale << std::endl;
    std::cout << "Auto-tuned streamline tube radius: " << config.tubeRadius << std::endl;
    std::cout << "Auto-tuned stream glyph scale: " << config.streamGlyphScale << std::endl;


    // Force the 3-component array to be treated as vector data.
    vtkAssignAttribute* vectorSource = vtkAssignAttribute::New();
    vectorSource->SetInputConnection(readerAlgorithm->GetOutputPort());
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


  // Noise-floor filter (3D datasets only, i.e. carotid): drop
  // background/near-static points entirely before hedgehog and
  // glyph masking, so they don't clutter the view with meaningless
  // "dust". 2D datasets pass straight through unfiltered.
    vtkSmartPointer<vtkThresholdPoints> flowThreshold =
        vtkSmartPointer<vtkThresholdPoints>::New();
    flowThreshold->SetInputConnection(vectorSource->GetOutputPort());
    flowThreshold->ThresholdByUpper(noiseFloor);

    vtkAlgorithmOutput* renderInputPort = config.is3D
        ? flowThreshold->GetOutputPort()
        : vectorSource->GetOutputPort();

    if (config.is3D)
    {
        // The old fixed glyphMaskRatio (100) was tuned for the full
        // ~167k-point dataset. Now that background noise is filtered
        // out first, that same ratio crushes the much smaller
        // remaining set down to almost nothing (a handful of cones).
        // Recompute the ratio from the ACTUAL filtered point count so
        // we land on a sensible number of cones regardless of how
        // large the filtered "real flow" region turns out to be.
        flowThreshold->Update();

        vtkIdType filteredCount = flowThreshold->GetOutput()->GetNumberOfPoints();

        std::cout << "Points remaining after noise-floor filter: "
                  << filteredCount << std::endl;

        const int desiredGlyphCount = 2000;
        int dynamicGlyphRatio = 1;

        if (filteredCount > desiredGlyphCount)
        {
            dynamicGlyphRatio = static_cast<int>(filteredCount / desiredGlyphCount);
        }

        config.glyphMaskRatio = dynamicGlyphRatio;

        std::cout << "Auto-tuned glyph mask ratio: "
                  << config.glyphMaskRatio << std::endl;

        // Hedgehog lines are cheap to render, but a dense 3D
        // volumetric grid (as opposed to a flat 2D grid like
        // testData1/testData2) still looks like a solid lattice box
        // if every single point gets a line - unlike carotid's much
        // smaller filtered "vessel-only" point count, an unfiltered
        // dataset like kitchen keeps its full ~11k points and needs
        // its own density cap.
        const int desiredHedgehogCount = 2500;
        int dynamicHedgehogRatio = 1;

        if (filteredCount > desiredHedgehogCount)
        {
            dynamicHedgehogRatio = static_cast<int>(filteredCount / desiredHedgehogCount);
        }

        config.hedgeMaskRatio = dynamicHedgehogRatio;

        std::cout << "Auto-tuned hedgehog mask ratio: "
                  << config.hedgeMaskRatio << std::endl;
    }


  // Hedgehog setup.
    vtkMaskPoints* hedgeMask = vtkMaskPoints::New();
    hedgeMask->SetInputConnection(renderInputPort);
    hedgeMask->SetOnRatio(config.hedgeMaskRatio);

    if (config.is3D)
    {
        // Random sampling avoids the diagonal striping artifact that
        // a fixed-stride OnRatio produces on structured 3D grids.
        hedgeMask->RandomModeOn();
    }
    else
    {
        hedgeMask->RandomModeOff();
    }

    vtkAssignAttribute* hedgeVectorSource = vtkAssignAttribute::New();
    hedgeVectorSource->SetInputConnection(hedgeMask->GetOutputPort());
    hedgeVectorSource->Assign(
        "VectorDisplay",
        vtkDataSetAttributes::VECTORS,
        vtkAssignAttribute::POINT_DATA
    );

    vtkHedgeHog *hhog = vtkHedgeHog::New();
    hhog->SetInputConnection(hedgeVectorSource->GetOutputPort());
    hhog->SetScaleFactor(config.hedgehogScale);


  // Glyph setup.
    vtkMaskPoints* glyphMask = vtkMaskPoints::New();
    glyphMask->SetInputConnection(renderInputPort);
    glyphMask->SetOnRatio(config.glyphMaskRatio);

    if (config.is3D)
    {
        glyphMask->RandomModeOn();
    }
    else
    {
        glyphMask->RandomModeOff();
    }

    vtkAssignAttribute* glyphVectorSource = vtkAssignAttribute::New();
    glyphVectorSource->SetInputConnection(glyphMask->GetOutputPort());
    glyphVectorSource->Assign(
        "VectorDisplay",
        vtkDataSetAttributes::VECTORS,
        vtkAssignAttribute::POINT_DATA
    );

    // Simple cone glyph (arrow substitute) instead of a complex arrow.
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
    glyph->SetColorModeToColorByScalar();
    glyph->OrientOn();


  // Streamline setup.
  // Seed points are placed on a regular grid spanning the exact
  // index ranges specified in the assignment for this dataset. For
  // carotid, candidates below the noise floor are skipped so seeds
  // only start in regions with real flow.
    vtkSmartPointer<vtkPolyData> seedPolyData =
        CreateRegularGridSeedPoints(
            vectorDataSet,
            pointData->GetArray("VectorMagnitude"),
            noiseFloor,
            config);

    // For 3D datasets where no background/signal gap was found (a
    // smooth, continuous flow field like kitchen's room convection,
    // as opposed to carotid's already-tuned vessel data), the fixed
    // per-dataset streamMaxPropagation guess has no way of knowing the
    // dataset's actual physical size. If it's too large relative to
    // the domain, streamlines loop through recirculating flow many
    // times over and turn into a solid tangled mass instead of clean
    // traceable lines. Auto-tune it from the real bounding box instead.
    if (config.is3D && !backgroundGapFound)
    {
        double bounds[6];
        vectorDataSet->GetBounds(bounds);

        double domainSize = 0.0;

        for (int d = 0; d < 3; d++)
        {
            double extent = bounds[2 * d + 1] - bounds[2 * d];

            if (extent > domainSize)
            {
                domainSize = extent;
            }
        }

        if (domainSize > 0.0)
        {
            // Enough propagation distance to trace through a couple of
            // recirculation loops without excessive self-overlap.
            config.streamMaxPropagation = domainSize * 2.5;
            config.streamInitialStep = gridSpacing * 0.5;

            std::cout << "Auto-tuned streamline max propagation: "
                      << config.streamMaxPropagation << std::endl;
            std::cout << "Auto-tuned streamline initial step: "
                      << config.streamInitialStep << std::endl;
        }
    }

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

    // For carotid, stop integrating once a streamline drifts down into
    // near-static background noise - otherwise RK4 keeps wandering
    // through near-zero (but not exactly zero) vectors indefinitely,
    // producing long chaotic tangles instead of clean vessel paths.
    // 2D datasets don't have this background-noise problem, so they
    // keep the effectively-disabled terminal speed.
    double terminalSpeed = config.is3D ? noiseFloor : 0.000000000001;
    streamTracer->SetTerminalSpeed(terminalSpeed);
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
  // This places cone glyphs along the generated streamline points,
  // using vtkStreamPoints-style sampling via vtkMaskPoints.
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
    streamGlyph->SetColorModeToColorByScalar();
    streamGlyph->OrientOn();


  // Colour range: for 3D datasets (carotid), background/near-static
  // points are now filtered out entirely, so use [noiseFloor, magMax]
  // to get good colour contrast across the remaining "real flow"
  // points instead of wasting the colour range on values that are
  // no longer even rendered. 2D datasets keep the full [magMin, magMax].
    double colorRangeMin = config.is3D ? noiseFloor : magMin;
    double colorRangeMax = magMax;

  // Lookup Table - this is the colour scale. Blue (slow) -> Red (fast).
    vtkLookupTable *lut = vtkLookupTable::New();
    lut->SetHueRange(0.667, 0.0);
    lut->SetTableRange(colorRangeMin, colorRangeMax);
    lut->Build();


  // Poly Mapper - scalar colouring turned ON, mapped by VectorMagnitude.
    vtkPolyDataMapper *hedgeMapper = vtkPolyDataMapper::New();
    hedgeMapper->SetInputConnection(hhog->GetOutputPort());
    hedgeMapper->SetLookupTable(lut);
    hedgeMapper->SetScalarModeToUsePointData();
    hedgeMapper->SelectColorArray("VectorMagnitude");
    hedgeMapper->SetScalarRange(colorRangeMin, colorRangeMax);
    hedgeMapper->ScalarVisibilityOn();

    vtkPolyDataMapper* glyphMapper = vtkPolyDataMapper::New();
    glyphMapper->SetInputConnection(glyph->GetOutputPort());
    glyphMapper->SetLookupTable(lut);
    glyphMapper->SetScalarModeToUsePointData();
    glyphMapper->SelectColorArray("VectorMagnitude");
    glyphMapper->SetScalarRange(colorRangeMin, colorRangeMax);
    glyphMapper->ScalarVisibilityOn();

    vtkPolyDataMapper* streamMapper = vtkPolyDataMapper::New();
    streamMapper->SetInputConnection(streamTube->GetOutputPort());
    streamMapper->SetLookupTable(lut);
    streamMapper->SetScalarModeToUsePointData();
    streamMapper->SelectColorArray("VectorMagnitude");
    streamMapper->SetScalarRange(colorRangeMin, colorRangeMax);
    streamMapper->ScalarVisibilityOn();

    vtkPolyDataMapper* streamGlyphMapper = vtkPolyDataMapper::New();
    streamGlyphMapper->SetInputConnection(streamGlyph->GetOutputPort());
    streamGlyphMapper->SetLookupTable(lut);
    streamGlyphMapper->SetScalarModeToUsePointData();
    streamGlyphMapper->SelectColorArray("VectorMagnitude");
    streamGlyphMapper->SetScalarRange(colorRangeMin, colorRangeMax);
    streamGlyphMapper->ScalarVisibilityOn();


  // Actor
    vtkActor *hedgeActor = vtkActor::New();
    hedgeActor->SetMapper(hedgeMapper);

    vtkActor* glyphActor = vtkActor::New();
    glyphActor->SetMapper(glyphMapper);

    vtkActor* streamActor = vtkActor::New();
    streamActor->SetMapper(streamMapper);

    vtkActor* streamGlyphActor = vtkActor::New();
    streamGlyphActor->SetMapper(streamGlyphMapper);


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
    std::cout << "1 : Hedgehog mode" << std::endl;
    std::cout << "2 : Cone glyph mode" << std::endl;
    std::cout << "3 : Streamline mode" << std::endl;
    std::cout << "4 : Streamline glyph mode" << std::endl;
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

    // pointsReader / gridReader are vtkSmartPointer - no manual Delete() needed.

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