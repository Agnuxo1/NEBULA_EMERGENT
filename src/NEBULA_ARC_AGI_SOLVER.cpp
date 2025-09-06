// NEBULA_ARC_AGI_SOLVER.cpp
// Complete ARC-AGI-2 Solver using NEBULA EMERGENT Architecture
// Spatial reasoning through emergent neural galaxy dynamics

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Json.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "RenderCore/Public/RenderGraphBuilder.h"
#include "NiagaraComponent.h"
#include <cuda_runtime.h>
#include <optix.h>

// ============================================================================
// ARC-AGI-2 Data Structures
// ============================================================================

struct FARCGrid
{
    TArray<TArray<int32>> Data;
    int32 Width;
    int32 Height;
    
    FARCGrid() : Width(0), Height(0) {}
    
    FARCGrid(int32 W, int32 H) : Width(W), Height(H)
    {
        Data.SetNum(H);
        for (int32 i = 0; i < H; i++)
        {
            Data[i].SetNum(W);
        }
    }
    
    int32 GetCell(int32 X, int32 Y) const
    {
        if (X >= 0 && X < Width && Y >= 0 && Y < Height)
            return Data[Y][X];
        return -1;
    }
    
    void SetCell(int32 X, int32 Y, int32 Value)
    {
        if (X >= 0 && X < Width && Y >= 0 && Y < Height)
            Data[Y][X] = Value;
    }
};

struct FARCExample
{
    FARCGrid Input;
    FARCGrid Output;
};

struct FARCTask
{
    FString TaskID;
    TArray<FARCExample> TrainingExamples;
    TArray<FARCExample> TestExamples;
};

// ============================================================================
// Neural Pattern Encoding for ARC
// ============================================================================

class FARCNeuralEncoder
{
private:
    // Color-to-wavelength mapping (10 ARC colors)
    TMap<int32, float> ColorWavelengthMap = {
        {0, 700e-9f},  // Black - Long wavelength (red)
        {1, 450e-9f},  // Blue
        {2, 650e-9f},  // Red  
        {3, 550e-9f},  // Green
        {4, 590e-9f},  // Yellow
        {5, 480e-9f},  // Grey
        {6, 600e-9f},  // Magenta
        {7, 610e-9f},  // Orange
        {8, 460e-9f},  // Light Blue
        {9, 520e-9f}   // Brown
    };
    
    // Spatial encoding parameters
    const float GRID_SCALE = 100.0f;  // Units per grid cell
    const float Z_LAYER_SPACING = 500.0f;  // Z spacing for different examples
    
public:
    struct FNeuralPattern
    {
        TArray<FVector3f> Positions;
        TArray<float> Wavelengths;
        TArray<float> Intensities;
        TArray<int32> GridIndices;  // Original grid position
        
        // Transformation hints discovered through evolution
        FTransform SpatialTransform;
        TArray<int32> ColorMapping;
    };
    
    FNeuralPattern EncodeGrid(const FARCGrid& Grid, int32 ExampleIndex = 0)
    {
        FNeuralPattern Pattern;
        
        for (int32 Y = 0; Y < Grid.Height; Y++)
        {
            for (int32 X = 0; X < Grid.Width; X++)
            {
                int32 Color = Grid.GetCell(X, Y);
                
                if (Color >= 0)  // Valid color
                {
                    // 3D position in neural space
                    FVector3f Position(
                        X * GRID_SCALE,
                        Y * GRID_SCALE,
                        ExampleIndex * Z_LAYER_SPACING
                    );
                    
                    Pattern.Positions.Add(Position);
                    Pattern.Wavelengths.Add(ColorWavelengthMap[Color]);
                    Pattern.Intensities.Add(1.0f + Color * 0.1f);  // Slight intensity variation
                    Pattern.GridIndices.Add(Y * Grid.Width + X);
                }
            }
        }
        
        return Pattern;
    }
    
    FARCGrid DecodePattern(const FNeuralPattern& Pattern, int32 Width, int32 Height)
    {
        FARCGrid Grid(Width, Height);
        
        // Initialize with background (0)
        for (int32 Y = 0; Y < Height; Y++)
        {
            for (int32 X = 0; X < Width; X++)
            {
                Grid.SetCell(X, Y, 0);
            }
        }
        
        // Decode neural activations back to grid
        for (int32 i = 0; i < Pattern.Positions.Num(); i++)
        {
            // Convert position back to grid coordinates
            int32 X = FMath::RoundToInt(Pattern.Positions[i].X / GRID_SCALE);
            int32 Y = FMath::RoundToInt(Pattern.Positions[i].Y / GRID_SCALE);
            
            // Convert wavelength back to color
            int32 Color = WavelengthToColor(Pattern.Wavelengths[i]);
            
            if (X >= 0 && X < Width && Y >= 0 && Y < Height)
            {
                Grid.SetCell(X, Y, Color);
            }
        }
        
        return Grid;
    }
    
private:
    int32 WavelengthToColor(float Wavelength)
    {
        // Find closest matching color
        int32 BestColor = 0;
        float MinDiff = FLT_MAX;
        
        for (const auto& Pair : ColorWavelengthMap)
        {
            float Diff = FMath::Abs(Wavelength - Pair.Value);
            if (Diff < MinDiff)
            {
                MinDiff = Diff;
                BestColor = Pair.Key;
            }
        }
        
        return BestColor;
    }
};

// ============================================================================
// Transformation Rule Discovery Engine
// ============================================================================

class FTransformationEngine
{
public:
    enum class ETransformationType
    {
        Translation,
        Rotation,
        Reflection,
        Scaling,
        ColorMapping,
        PatternFill,
        Symmetry,
        Connectivity,
        Enclosure,
        Counting
    };
    
    struct FTransformationRule
    {
        ETransformationType Type;
        FTransform SpatialTransform;
        TMap<int32, int32> ColorMap;
        TFunction<FARCGrid(const FARCGrid&)> CustomTransform;
        float Confidence;
        
        FARCGrid Apply(const FARCGrid& Input) const
        {
            switch (Type)
            {
                case ETransformationType::Translation:
                    return ApplyTranslation(Input);
                case ETransformationType::Rotation:
                    return ApplyRotation(Input);
                case ETransformationType::Reflection:
                    return ApplyReflection(Input);
                case ETransformationType::ColorMapping:
                    return ApplyColorMapping(Input);
                default:
                    if (CustomTransform)
                        return CustomTransform(Input);
                    return Input;
            }
        }
        
    private:
        FARCGrid ApplyTranslation(const FARCGrid& Input) const
        {
            FARCGrid Output(Input.Width, Input.Height);
            
            int32 DX = FMath::RoundToInt(SpatialTransform.GetTranslation().X);
            int32 DY = FMath::RoundToInt(SpatialTransform.GetTranslation().Y);
            
            for (int32 Y = 0; Y < Input.Height; Y++)
            {
                for (int32 X = 0; X < Input.Width; X++)
                {
                    int32 NewX = X + DX;
                    int32 NewY = Y + DY;
                    
                    if (NewX >= 0 && NewX < Input.Width && NewY >= 0 && NewY < Input.Height)
                    {
                        Output.SetCell(NewX, NewY, Input.GetCell(X, Y));
                    }
                }
            }
            
            return Output;
        }
        
        FARCGrid ApplyRotation(const FARCGrid& Input) const
        {
            // 90-degree rotation
            FARCGrid Output(Input.Height, Input.Width);
            
            for (int32 Y = 0; Y < Input.Height; Y++)
            {
                for (int32 X = 0; X < Input.Width; X++)
                {
                    Output.SetCell(Input.Height - 1 - Y, X, Input.GetCell(X, Y));
                }
            }
            
            return Output;
        }
        
        FARCGrid ApplyReflection(const FARCGrid& Input) const
        {
            FARCGrid Output(Input.Width, Input.Height);
            
            bool bHorizontal = SpatialTransform.GetRotation().Euler().X > 0;
            
            for (int32 Y = 0; Y < Input.Height; Y++)
            {
                for (int32 X = 0; X < Input.Width; X++)
                {
                    if (bHorizontal)
                        Output.SetCell(Input.Width - 1 - X, Y, Input.GetCell(X, Y));
                    else
                        Output.SetCell(X, Input.Height - 1 - Y, Input.GetCell(X, Y));
                }
            }
            
            return Output;
        }
        
        FARCGrid ApplyColorMapping(const FARCGrid& Input) const
        {
            FARCGrid Output(Input.Width, Input.Height);
            
            for (int32 Y = 0; Y < Input.Height; Y++)
            {
                for (int32 X = 0; X < Input.Width; X++)
                {
                    int32 OldColor = Input.GetCell(X, Y);
                    int32 NewColor = ColorMap.Contains(OldColor) ? ColorMap[OldColor] : OldColor;
                    Output.SetCell(X, Y, NewColor);
                }
            }
            
            return Output;
        }
    };
    
    TArray<FTransformationRule> DiscoverRules(const TArray<FARCExample>& Examples)
    {
        TArray<FTransformationRule> Rules;
        
        // Analyze each example pair
        for (const FARCExample& Example : Examples)
        {
            // Try different transformation types
            TryDiscoverTranslation(Example, Rules);
            TryDiscoverRotation(Example, Rules);
            TryDiscoverReflection(Example, Rules);
            TryDiscoverColorMapping(Example, Rules);
            TryDiscoverPatternFill(Example, Rules);
            TryDiscoverSymmetry(Example, Rules);
            TryDiscoverConnectivity(Example, Rules);
        }
        
        // Combine and validate rules across all examples
        ValidateRulesAcrossExamples(Rules, Examples);
        
        return Rules;
    }
    
private:
    void TryDiscoverTranslation(const FARCExample& Example, TArray<FTransformationRule>& Rules)
    {
        // Find objects in input and output
        TArray<FIntRect> InputObjects = FindObjects(Example.Input);
        TArray<FIntRect> OutputObjects = FindObjects(Example.Output);
        
        if (InputObjects.Num() == OutputObjects.Num() && InputObjects.Num() > 0)
        {
            // Calculate average translation
            FVector2D AvgTranslation(0, 0);
            
            for (int32 i = 0; i < InputObjects.Num(); i++)
            {
                FVector2D InputCenter = GetRectCenter(InputObjects[i]);
                FVector2D OutputCenter = GetRectCenter(OutputObjects[i]);
                AvgTranslation += OutputCenter - InputCenter;
            }
            
            AvgTranslation /= InputObjects.Num();
            
            if (AvgTranslation.Size() > 0.1f)
            {
                FTransformationRule Rule;
                Rule.Type = ETransformationType::Translation;
                Rule.SpatialTransform.SetTranslation(FVector(AvgTranslation.X, AvgTranslation.Y, 0));
                Rule.Confidence = 0.8f;
                Rules.Add(Rule);
            }
        }
    }
    
    void TryDiscoverColorMapping(const FARCExample& Example, TArray<FTransformationRule>& Rules)
    {
        TMap<int32, int32> ColorMap;
        TMap<int32, int32> ColorCounts;
        
        // Count color frequencies
        for (int32 Y = 0; Y < FMath::Min(Example.Input.Height, Example.Output.Height); Y++)
        {
            for (int32 X = 0; X < FMath::Min(Example.Input.Width, Example.Output.Width); X++)
            {
                int32 InputColor = Example.Input.GetCell(X, Y);
                int32 OutputColor = Example.Output.GetCell(X, Y);
                
                if (InputColor != OutputColor && InputColor >= 0 && OutputColor >= 0)
                {
                    if (!ColorMap.Contains(InputColor))
                    {
                        ColorMap.Add(InputColor, OutputColor);
                        ColorCounts.Add(InputColor, 1);
                    }
                    else if (ColorMap[InputColor] == OutputColor)
                    {
                        ColorCounts[InputColor]++;
                    }
                }
            }
        }
        
        // Create rule if consistent mapping found
        if (ColorMap.Num() > 0)
        {
            FTransformationRule Rule;
            Rule.Type = ETransformationType::ColorMapping;
            Rule.ColorMap = ColorMap;
            Rule.Confidence = 0.7f;
            Rules.Add(Rule);
        }
    }
    
    void TryDiscoverSymmetry(const FARCExample& Example, TArray<FTransformationRule>& Rules)
    {
        // Check for symmetry operations
        bool bHorizontalSymmetry = CheckHorizontalSymmetry(Example.Output);
        bool bVerticalSymmetry = CheckVerticalSymmetry(Example.Output);
        bool bDiagonalSymmetry = CheckDiagonalSymmetry(Example.Output);
        
        if (bHorizontalSymmetry || bVerticalSymmetry || bDiagonalSymmetry)
        {
            FTransformationRule Rule;
            Rule.Type = ETransformationType::Symmetry;
            Rule.CustomTransform = [=](const FARCGrid& Input) -> FARCGrid
            {
                return ApplySymmetry(Input, bHorizontalSymmetry, bVerticalSymmetry, bDiagonalSymmetry);
            };
            Rule.Confidence = 0.9f;
            Rules.Add(Rule);
        }
    }
    
    void TryDiscoverConnectivity(const FARCExample& Example, TArray<FTransformationRule>& Rules)
    {
        // Analyze connected components
        TArray<TArray<FIntPoint>> InputComponents = FindConnectedComponents(Example.Input);
        TArray<TArray<FIntPoint>> OutputComponents = FindConnectedComponents(Example.Output);
        
        // Check if output connects previously disconnected components
        if (OutputComponents.Num() < InputComponents.Num())
        {
            FTransformationRule Rule;
            Rule.Type = ETransformationType::Connectivity;
            Rule.CustomTransform = [](const FARCGrid& Input) -> FARCGrid
            {
                return ConnectComponents(Input);
            };
            Rule.Confidence = 0.75f;
            Rules.Add(Rule);
        }
    }
    
    void TryDiscoverPatternFill(const FARCExample& Example, TArray<FTransformationRule>& Rules)
    {
        // Check if output fills regions with patterns
        TArray<FIntRect> EmptyRegions = FindEmptyRegions(Example.Input);
        
        for (const FIntRect& Region : EmptyRegions)
        {
            // Check if this region has a pattern in output
            FARCGrid Pattern = ExtractRegion(Example.Output, Region);
            
            if (HasRepeatingPattern(Pattern))
            {
                FTransformationRule Rule;
                Rule.Type = ETransformationType::PatternFill;
                Rule.CustomTransform = [Pattern, Region](const FARCGrid& Input) -> FARCGrid
                {
                    return FillWithPattern(Input, Pattern, Region);
                };
                Rule.Confidence = 0.85f;
                Rules.Add(Rule);
            }
        }
    }
    
    // Helper functions
    TArray<FIntRect> FindObjects(const FARCGrid& Grid)
    {
        TArray<FIntRect> Objects;
        TArray<TArray<bool>> Visited;
        Visited.SetNum(Grid.Height);
        for (int32 i = 0; i < Grid.Height; i++)
        {
            Visited[i].SetNum(Grid.Width);
            for (int32 j = 0; j < Grid.Width; j++)
                Visited[i][j] = false;
        }
        
        for (int32 Y = 0; Y < Grid.Height; Y++)
        {
            for (int32 X = 0; X < Grid.Width; X++)
            {
                if (!Visited[Y][X] && Grid.GetCell(X, Y) != 0)
                {
                    FIntRect Object = FloodFillObject(Grid, X, Y, Visited);
                    Objects.Add(Object);
                }
            }
        }
        
        return Objects;
    }
    
    FIntRect FloodFillObject(const FARCGrid& Grid, int32 StartX, int32 StartY, TArray<TArray<bool>>& Visited)
    {
        FIntRect Bounds(StartX, StartY, StartX, StartY);
        TQueue<FIntPoint> Queue;
        Queue.Enqueue(FIntPoint(StartX, StartY));
        
        int32 TargetColor = Grid.GetCell(StartX, StartY);
        
        while (!Queue.IsEmpty())
        {
            FIntPoint Point;
            Queue.Dequeue(Point);
            
            if (Point.X < 0 || Point.X >= Grid.Width || Point.Y < 0 || Point.Y >= Grid.Height)
                continue;
                
            if (Visited[Point.Y][Point.X])
                continue;
                
            if (Grid.GetCell(Point.X, Point.Y) != TargetColor)
                continue;
            
            Visited[Point.Y][Point.X] = true;
            
            Bounds.Min.X = FMath::Min(Bounds.Min.X, Point.X);
            Bounds.Min.Y = FMath::Min(Bounds.Min.Y, Point.Y);
            Bounds.Max.X = FMath::Max(Bounds.Max.X, Point.X);
            Bounds.Max.Y = FMath::Max(Bounds.Max.Y, Point.Y);
            
            Queue.Enqueue(FIntPoint(Point.X + 1, Point.Y));
            Queue.Enqueue(FIntPoint(Point.X - 1, Point.Y));
            Queue.Enqueue(FIntPoint(Point.X, Point.Y + 1));
            Queue.Enqueue(FIntPoint(Point.X, Point.Y - 1));
        }
        
        return Bounds;
    }
    
    TArray<TArray<FIntPoint>> FindConnectedComponents(const FARCGrid& Grid)
    {
        TArray<TArray<FIntPoint>> Components;
        TArray<TArray<bool>> Visited;
        Visited.SetNum(Grid.Height);
        for (int32 i = 0; i < Grid.Height; i++)
        {
            Visited[i].SetNum(Grid.Width);
        }
        
        for (int32 Y = 0; Y < Grid.Height; Y++)
        {
            for (int32 X = 0; X < Grid.Width; X++)
            {
                if (!Visited[Y][X] && Grid.GetCell(X, Y) != 0)
                {
                    TArray<FIntPoint> Component;
                    DFS(Grid, X, Y, Visited, Component);
                    Components.Add(Component);
                }
            }
        }
        
        return Components;
    }
    
    void DFS(const FARCGrid& Grid, int32 X, int32 Y, TArray<TArray<bool>>& Visited, TArray<FIntPoint>& Component)
    {
        if (X < 0 || X >= Grid.Width || Y < 0 || Y >= Grid.Height || Visited[Y][X])
            return;
            
        if (Grid.GetCell(X, Y) == 0)
            return;
        
        Visited[Y][X] = true;
        Component.Add(FIntPoint(X, Y));
        
        DFS(Grid, X + 1, Y, Visited, Component);
        DFS(Grid, X - 1, Y, Visited, Component);
        DFS(Grid, X, Y + 1, Visited, Component);
        DFS(Grid, X, Y - 1, Visited, Component);
    }
    
    bool CheckHorizontalSymmetry(const FARCGrid& Grid)
    {
        for (int32 Y = 0; Y < Grid.Height / 2; Y++)
        {
            for (int32 X = 0; X < Grid.Width; X++)
            {
                if (Grid.GetCell(X, Y) != Grid.GetCell(X, Grid.Height - 1 - Y))
                    return false;
            }
        }
        return true;
    }
    
    bool CheckVerticalSymmetry(const FARCGrid& Grid)
    {
        for (int32 Y = 0; Y < Grid.Height; Y++)
        {
            for (int32 X = 0; X < Grid.Width / 2; X++)
            {
                if (Grid.GetCell(X, Y) != Grid.GetCell(Grid.Width - 1 - X, Y))
                    return false;
            }
        }
        return true;
    }
    
    bool CheckDiagonalSymmetry(const FARCGrid& Grid)
    {
        if (Grid.Width != Grid.Height)
            return false;
            
        for (int32 Y = 0; Y < Grid.Height; Y++)
        {
            for (int32 X = 0; X < Y; X++)
            {
                if (Grid.GetCell(X, Y) != Grid.GetCell(Y, X))
                    return false;
            }
        }
        return true;
    }
    
    FARCGrid ApplySymmetry(const FARCGrid& Input, bool bHorizontal, bool bVertical, bool bDiagonal)
    {
        FARCGrid Output = Input;
        
        if (bHorizontal)
        {
            for (int32 Y = 0; Y < Input.Height / 2; Y++)
            {
                for (int32 X = 0; X < Input.Width; X++)
                {
                    int32 Value = Input.GetCell(X, Y);
                    Output.SetCell(X, Input.Height - 1 - Y, Value);
                }
            }
        }
        
        if (bVertical)
        {
            for (int32 Y = 0; Y < Input.Height; Y++)
            {
                for (int32 X = 0; X < Input.Width / 2; X++)
                {
                    int32 Value = Input.GetCell(X, Y);
                    Output.SetCell(Input.Width - 1 - X, Y, Value);
                }
            }
        }
        
        return Output;
    }
    
    FARCGrid ConnectComponents(const FARCGrid& Input)
    {
        FARCGrid Output = Input;
        TArray<TArray<FIntPoint>> Components = FindConnectedComponents(Input);
        
        // Connect nearest components
        for (int32 i = 0; i < Components.Num() - 1; i++)
        {
            for (int32 j = i + 1; j < Components.Num(); j++)
            {
                FIntPoint P1 = GetComponentCenter(Components[i]);
                FIntPoint P2 = GetComponentCenter(Components[j]);
                
                // Draw line between centers
                DrawLine(Output, P1, P2, 1);  // Use color 1 for connections
            }
        }
        
        return Output;
    }
    
    void DrawLine(FARCGrid& Grid, FIntPoint Start, FIntPoint End, int32 Color)
    {
        // Bresenham's line algorithm
        int32 X0 = Start.X, Y0 = Start.Y;
        int32 X1 = End.X, Y1 = End.Y;
        
        int32 DX = FMath::Abs(X1 - X0);
        int32 DY = FMath::Abs(Y1 - Y0);
        int32 SX = X0 < X1 ? 1 : -1;
        int32 SY = Y0 < Y1 ? 1 : -1;
        int32 Err = DX - DY;
        
        while (true)
        {
            Grid.SetCell(X0, Y0, Color);
            
            if (X0 == X1 && Y0 == Y1)
                break;
                
            int32 E2 = 2 * Err;
            if (E2 > -DY)
            {
                Err -= DY;
                X0 += SX;
            }
            if (E2 < DX)
            {
                Err += DX;
                Y0 += SY;
            }
        }
    }
    
    FIntPoint GetComponentCenter(const TArray<FIntPoint>& Component)
    {
        FIntPoint Center(0, 0);
        for (const FIntPoint& Point : Component)
        {
            Center.X += Point.X;
            Center.Y += Point.Y;
        }
        Center.X /= Component.Num();
        Center.Y /= Component.Num();
        return Center;
    }
    
    void ValidateRulesAcrossExamples(TArray<FTransformationRule>& Rules, const TArray<FARCExample>& Examples)
    {
        for (FTransformationRule& Rule : Rules)
        {
            int32 SuccessCount = 0;
            
            for (const FARCExample& Example : Examples)
            {
                FARCGrid Predicted = Rule.Apply(Example.Input);
                
                if (GridsMatch(Predicted, Example.Output))
                {
                    SuccessCount++;
                }
            }
            
            Rule.Confidence = (float)SuccessCount / Examples.Num();
        }
        
        // Remove low-confidence rules
        Rules.RemoveAll([](const FTransformationRule& Rule) { return Rule.Confidence < 0.5f; });
        
        // Sort by confidence
        Rules.Sort([](const FTransformationRule& A, const FTransformationRule& B) {
            return A.Confidence > B.Confidence;
        });
    }
    
    bool GridsMatch(const FARCGrid& A, const FARCGrid& B)
    {
        if (A.Width != B.Width || A.Height != B.Height)
            return false;
            
        for (int32 Y = 0; Y < A.Height; Y++)
        {
            for (int32 X = 0; X < A.Width; X++)
            {
                if (A.GetCell(X, Y) != B.GetCell(X, Y))
                    return false;
            }
        }
        
        return true;
    }
};

// ============================================================================
// NEBULA ARC-AGI Solver
// ============================================================================

UCLASS()
class ARC_API ANebulaARCSolver : public AActor
{
    GENERATED_BODY()
    
public:
    ANebulaARCSolver();
    
    // Main solving interface
    UFUNCTION(BlueprintCallable, Category = "ARC Solver")
    FARCGrid SolveTask(const FString& TaskJSON);
    
    UFUNCTION(BlueprintCallable, Category = "ARC Solver")
    void LoadAllTasks(const FString& FolderPath);
    
    UFUNCTION(BlueprintCallable, Category = "ARC Solver")
    void TrainOnExamples(const TArray<FARCExample>& Examples);
    
protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    
private:
    // Core components
    UPROPERTY()
    class ANebulaEmergent* NebulaCore;
    
    UPROPERTY()
    class UNiagaraComponent* VisualizationSystem;
    
    // ARC-specific modules
    FARCNeuralEncoder Encoder;
    FTransformationEngine TransformEngine;
    
    // Task data
    TArray<FARCTask> LoadedTasks;
    FARCTask CurrentTask;
    
    // GPU buffers for pattern processing
    TRefCountPtr<FRDGPooledBuffer> PatternBuffer;
    TRefCountPtr<FRDGPooledBuffer> TransformBuffer;
    
    // Evolution parameters
    int32 EvolutionCycles = 100;
    float MutationRate = 0.1f;
    float CrossoverRate = 0.7f;
    
    // Methods
    FARCTask ParseTaskJSON(const FString& JSON);
    void InjectPatternsIntoNebula(const TArray<FARCNeuralEncoder::FNeuralPattern>& Patterns);
    TArray<FTransformationEngine::FTransformationRule> EvolveTransformationRules();
    FARCGrid ApplyBestRule(const FARCGrid& Input, const TArray<FTransformationEngine::FTransformationRule>& Rules);
    
    // GPU compute shaders
    void DispatchPatternEvolution(FRHICommandList& RHICmdList);
    void DispatchRuleDiscovery(FRHICommandList& RHICmdList);
};

// Implementation

ANebulaARCSolver::ANebulaARCSolver()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // Create visualization component
    VisualizationSystem = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ARCVisualization"));
    RootComponent = VisualizationSystem;
}

void ANebulaARCSolver::BeginPlay()
{
    Super::BeginPlay();
    
    // Spawn NEBULA core
    FActorSpawnParameters SpawnParams;
    NebulaCore = GetWorld()->SpawnActor<ANebulaEmergent>(ANebulaEmergent::StaticClass(), SpawnParams);
    
    if (NebulaCore)
    {
        // Configure for ARC solving
        NebulaCore->NumNeurons = 500000;  // Reduced for faster iteration
        NebulaCore->PhotonsPerNeuron = 50;
        NebulaCore->FusionThreshold = 2.0f;  // Higher threshold for pattern stability
        
        UE_LOG(LogTemp, Warning, TEXT("NEBULA ARC Solver initialized"));
    }
}

FARCGrid ANebulaARCSolver::SolveTask(const FString& TaskJSON)
{
    // Parse task
    CurrentTask = ParseTaskJSON(TaskJSON);
    
    // Encode training examples into neural patterns
    TArray<FARCNeuralEncoder::FNeuralPattern> TrainingPatterns;
    
    for (const FARCExample& Example : CurrentTask.TrainingExamples)
    {
        FARCNeuralEncoder::FNeuralPattern InputPattern = Encoder.EncodeGrid(Example.Input);
        FARCNeuralEncoder::FNeuralPattern OutputPattern = Encoder.EncodeGrid(Example.Output);
        
        TrainingPatterns.Add(InputPattern);
        TrainingPatterns.Add(OutputPattern);
    }
    
    // Inject patterns into NEBULA
    InjectPatternsIntoNebula(TrainingPatterns);
    
    // Let NEBULA evolve to discover patterns
    for (int32 Cycle = 0; Cycle < EvolutionCycles; Cycle++)
    {
        NebulaCore->EvolveFrame(0.016f);  // 60 FPS
        
        // Every 10 cycles, attempt rule extraction
        if (Cycle % 10 == 0)
        {
            TArray<FTransformationEngine::FTransformationRule> Rules = EvolveTransformationRules();
            
            // Test rules on training examples
            bool bAllCorrect = true;
            for (const FARCExample& Example : CurrentTask.TrainingExamples)
            {
                FARCGrid Predicted = ApplyBestRule(Example.Input, Rules);
                if (!GridsMatch(Predicted, Example.Output))
                {
                    bAllCorrect = false;
                    break;
                }
            }
            
            if (bAllCorrect)
            {
                // Apply to test case
                if (CurrentTask.TestExamples.Num() > 0)
                {
                    return ApplyBestRule(CurrentTask.TestExamples[0].Input, Rules);
                }
            }
        }
    }
    
    // Fallback: use transformation engine directly
    TArray<FTransformationEngine::FTransformationRule> Rules = 
        TransformEngine.DiscoverRules(CurrentTask.TrainingExamples);
    
    if (CurrentTask.TestExamples.Num() > 0)
    {
        return ApplyBestRule(CurrentTask.TestExamples[0].Input, Rules);
    }
    
    return FARCGrid();
}

void ANebulaARCSolver::InjectPatternsIntoNebula(const TArray<FARCNeuralEncoder::FNeuralPattern>& Patterns)
{
    // Convert patterns to photon emissions
    for (const FARCNeuralEncoder::FNeuralPattern& Pattern : Patterns)
    {
        TArray<FPhotonBurst> Bursts;
        
        for (int32 i = 0; i < Pattern.Positions.Num(); i++)
        {
            FPhotonBurst Burst;
            Burst.OriginPosition = Pattern.Positions[i];
            
            // Create photons with pattern-specific wavelengths
            for (int32 j = 0; j < 10; j++)
            {
                FPhotonEmission Photon;
                Photon.Wavelength = Pattern.Wavelengths[i];
                Photon.Energy = Pattern.Intensities[i] * 1e-15f;
                Photon.Direction = FVector3f::RandomUnit();
                Burst.Photons.Add(Photon);
            }
            
            Bursts.Add(Burst);
        }
        
        NebulaCore->InjectPhotonBursts(Bursts);
    }
}

TArray<FTransformationEngine::FTransformationRule> ANebulaARCSolver::EvolveTransformationRules()
{
    // Extract active neuron clusters from NEBULA
    TArray<FNeuronCluster> Clusters = NebulaCore->GetActiveClusters();
    
    // Analyze cluster patterns for transformation rules
    TArray<FTransformationEngine::FTransformationRule> Rules;
    
    for (const FNeuronCluster& Cluster : Clusters)
    {
        // Analyze spatial distribution
        FVector Center, Extents;
        Cluster.GetBounds(Center, Extents);
        
        // Check for translation pattern
        if (Cluster.Velocity.Size() > 0.1f)
        {
            FTransformationEngine::FTransformationRule Rule;
            Rule.Type = FTransformationEngine::ETransformationType::Translation;
            Rule.SpatialTransform.SetTranslation(Cluster.Velocity * 10.0f);
            Rule.Confidence = Cluster.Coherence;
            Rules.Add(Rule);
        }
        
        // Check for rotation pattern
        if (FMath::Abs(Cluster.AngularVelocity.Z) > 0.01f)
        {
            FTransformationEngine::FTransformationRule Rule;
            Rule.Type = FTransformationEngine::ETransformationType::Rotation;
            Rule.SpatialTransform.SetRotation(FQuat(FVector::UpVector, Cluster.AngularVelocity.Z));
            Rule.Confidence = Cluster.Coherence;
            Rules.Add(Rule);
        }
        
        // Check for color mapping based on spectral analysis
        TMap<float, float> SpectralMap = Cluster.GetSpectralMapping();
        if (SpectralMap.Num() > 1)
        {
            FTransformationEngine::FTransformationRule Rule;
            Rule.Type = FTransformationEngine::ETransformationType::ColorMapping;
            
            // Convert wavelengths back to colors
            for (const auto& Pair : SpectralMap)
            {
                int32 FromColor = Encoder.WavelengthToColor(Pair.Key);
                int32 ToColor = Encoder.WavelengthToColor(Pair.Value);
                Rule.ColorMap.Add(FromColor, ToColor);
            }
            
            Rule.Confidence = Cluster.Coherence;
            Rules.Add(Rule);
        }
    }
    
    return Rules;
}

FARCGrid ANebulaARCSolver::ApplyBestRule(const FARCGrid& Input, 
                                         const TArray<FTransformationEngine::FTransformationRule>& Rules)
{
    if (Rules.Num() == 0)
        return Input;
    
    // Apply rule with highest confidence
    FARCGrid Result = Rules[0].Apply(Input);
    
    // Chain multiple rules if needed
    for (int32 i = 1; i < FMath::Min(3, Rules.Num()); i++)
    {
        if (Rules[i].Confidence > 0.7f)
        {
            Result = Rules[i].Apply(Result);
        }
    }
    
    return Result;
}

FARCTask ANebulaARCSolver::ParseTaskJSON(const FString& JSON)
{
    FARCTask Task;
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSON);
    
    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        // Parse training examples
        const TArray<TSharedPtr<FJsonValue>>* TrainArray;
        if (JsonObject->TryGetArrayField(TEXT("train"), TrainArray))
        {
            for (const TSharedPtr<FJsonValue>& Value : *TrainArray)
            {
                FARCExample Example;
                TSharedPtr<FJsonObject> ExampleObj = Value->AsObject();
                
                // Parse input grid
                const TArray<TSharedPtr<FJsonValue>>* InputArray;
                if (ExampleObj->TryGetArrayField(TEXT("input"), InputArray))
                {
                    Example.Input = ParseGrid(*InputArray);
                }
                
                // Parse output grid
                const TArray<TSharedPtr<FJsonValue>>* OutputArray;
                if (ExampleObj->TryGetArrayField(TEXT("output"), OutputArray))
                {
                    Example.Output = ParseGrid(*OutputArray);
                }
                
                Task.TrainingExamples.Add(Example);
            }
        }
        
        // Parse test examples
        const TArray<TSharedPtr<FJsonValue>>* TestArray;
        if (JsonObject->TryGetArrayField(TEXT("test"), TestArray))
        {
            for (const TSharedPtr<FJsonValue>& Value : *TestArray)
            {
                FARCExample Example;
                TSharedPtr<FJsonObject> ExampleObj = Value->AsObject();
                
                const TArray<TSharedPtr<FJsonValue>>* InputArray;
                if (ExampleObj->TryGetArrayField(TEXT("input"), InputArray))
                {
                    Example.Input = ParseGrid(*InputArray);
                }
                
                Task.TestExamples.Add(Example);
            }
        }
    }
    
    return Task;
}

FARCGrid ANebulaARCSolver::ParseGrid(const TArray<TSharedPtr<FJsonValue>>& JsonArray)
{
    FARCGrid Grid;
    Grid.Height = JsonArray.Num();
    
    if (Grid.Height > 0)
    {
        const TArray<TSharedPtr<FJsonValue>>* FirstRow;
        if (JsonArray[0]->TryGetArray(FirstRow))
        {
            Grid.Width = FirstRow->Num();
        }
        
        Grid.Data.SetNum(Grid.Height);
        
        for (int32 Y = 0; Y < Grid.Height; Y++)
        {
            const TArray<TSharedPtr<FJsonValue>>* Row;
            if (JsonArray[Y]->TryGetArray(Row))
            {
                Grid.Data[Y].SetNum(Grid.Width);
                
                for (int32 X = 0; X < Grid.Width; X++)
                {
                    Grid.Data[Y][X] = (*Row)[X]->AsNumber();
                }
            }
        }
    }
    
    return Grid;
}

void ANebulaARCSolver::LoadAllTasks(const FString& FolderPath)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    TArray<FString> TaskFiles;
    PlatformFile.FindFilesRecursively(TaskFiles, *FolderPath, TEXT("*.json"));
    
    for (const FString& FilePath : TaskFiles)
    {
        FString FileContent;
        if (FFileHelper::LoadFileToString(FileContent, *FilePath))
        {
            FARCTask Task = ParseTaskJSON(FileContent);
            Task.TaskID = FPaths::GetBaseFilename(FilePath);
            LoadedTasks.Add(Task);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Loaded %d ARC tasks"), LoadedTasks.Num());
}

void ANebulaARCSolver::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Continuous evolution if solving
    if (NebulaCore && CurrentTask.TrainingExamples.Num() > 0)
    {
        NebulaCore->EvolveFrame(DeltaTime);
    }
}

// ============================================================================
// Main Execution Entry Point
// ============================================================================

class FARCCompetitionRunner
{
public:
    static void RunCompetition(const FString& DataPath, const FString& OutputPath)
    {
        UE_LOG(LogTemp, Warning, TEXT("Starting ARC-AGI-2 Competition"));
        
        // Create solver
        UWorld* World = GEngine->GetWorldContexts()[0].World();
        ANebulaARCSolver* Solver = World->SpawnActor<ANebulaARCSolver>();
        
        // Load all tasks
        Solver->LoadAllTasks(DataPath);
        
        // Solution storage
        TMap<FString, FARCGrid> Solutions;
        
        // Process each task
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        TArray<FString> TaskFiles;
        PlatformFile.FindFilesRecursively(TaskFiles, *DataPath, TEXT("*.json"));
        
        for (const FString& TaskFile : TaskFiles)
        {
            FString TaskJSON;
            if (FFileHelper::LoadFileToString(TaskJSON, *TaskFile))
            {
                FString TaskID = FPaths::GetBaseFilename(TaskFile);
                UE_LOG(LogTemp, Warning, TEXT("Solving task: %s"), *TaskID);
                
                FARCGrid Solution = Solver->SolveTask(TaskJSON);
                Solutions.Add(TaskID, Solution);
            }
        }
        
        // Generate submission
        GenerateSubmission(Solutions, OutputPath);
    }
    
private:
    static void GenerateSubmission(const TMap<FString, FARCGrid>& Solutions, const FString& OutputPath)
    {
        TSharedPtr<FJsonObject> Submission = MakeShareable(new FJsonObject);
        
        for (const auto& Pair : Solutions)
        {
            TArray<TSharedPtr<FJsonValue>> GridArray;
            
            for (int32 Y = 0; Y < Pair.Value.Height; Y++)
            {
                TArray<TSharedPtr<FJsonValue>> RowArray;
                
                for (int32 X = 0; X < Pair.Value.Width; X++)
                {
                    RowArray.Add(MakeShareable(new FJsonValueNumber(Pair.Value.GetCell(X, Y))));
                }
                
                GridArray.Add(MakeShareable(new FJsonValueArray(RowArray)));
            }
            
            Submission->SetArrayField(Pair.Key, GridArray);
        }
        
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(Submission.ToSharedRef(), Writer);
        
        FFileHelper::SaveStringToFile(OutputString, *OutputPath);
        
        UE_LOG(LogTemp, Warning, TEXT("Submission saved to: %s"), *OutputPath);
    }
};