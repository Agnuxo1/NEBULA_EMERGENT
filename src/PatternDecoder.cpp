// Pattern-to-Rule Decoder Implementation
// PatternDecoder.cpp

class FPatternToRuleDecoder
{
private:
    struct FClusterPattern
    {
        TArray<FVector3f> Positions;
        TArray<float> Activations;
        TArray<float> Wavelengths;
        FVector3f Velocity;
        float AngularMomentum;
        float TotalEnergy;
        
        // Computed features
        FTransform BoundingBox;
        float Coherence;
        float Symmetry;
        TArray<int32> SymmetryAxes;
    };
    
public:
    FTransformationRule DecodeClusterToRule(const FClusterPattern& Pattern)
    {
        FTransformationRule Rule;
        
        // Analyze spatial distribution
        FVector3f Center, Extents;
        CalculateBounds(Pattern.Positions, Center, Extents);
        
        // 1. TRANSLATION: Coherent linear motion
        if (Pattern.Velocity.Size() > 0.1f && Pattern.Coherence > 0.8f)
        {
            Rule.Type = ETransformationType::Translation;
            
            // Map velocity to grid units
            // Velocity in galaxy units -> grid cells
            float GridScale = 100.0f;  // From encoder
            FVector2D Translation(
                Pattern.Velocity.X / GridScale,
                Pattern.Velocity.Y / GridScale
            );
            
            Rule.SpatialTransform.SetTranslation(FVector(Translation.X, Translation.Y, 0));
            Rule.Confidence = Pattern.Coherence;
            
            return Rule;
        }
        
        // 2. ROTATION: Angular momentum pattern
        if (FMath::Abs(Pattern.AngularMomentum) > 0.01f)
        {
            Rule.Type = ETransformationType::Rotation;
            
            // Determine rotation angle from angular pattern
            float RotationAngle = DetermineRotationAngle(Pattern);
            
            Rule.SpatialTransform.SetRotation(
                FQuat(FVector::UpVector, RotationAngle)
            );
            Rule.Confidence = CalculateRotationalCoherence(Pattern);
            
            return Rule;
        }
        
        // 3. REFLECTION: Symmetric activation pattern
        if (Pattern.Symmetry > 0.9f)
        {
            Rule.Type = ETransformationType::Reflection;
            
            // Determine reflection axis from symmetry analysis
            if (Pattern.SymmetryAxes.Contains(0))  // Horizontal
            {
                Rule.CustomTransform = [](const FARCGrid& Input) -> FARCGrid
                {
                    return ReflectHorizontal(Input);
                };
            }
            else if (Pattern.SymmetryAxes.Contains(1))  // Vertical
            {
                Rule.CustomTransform = [](const FARCGrid& Input) -> FARCGrid
                {
                    return ReflectVertical(Input);
                };
            }
            
            Rule.Confidence = Pattern.Symmetry;
            return Rule;
        }
        
        // 4. COLOR MAPPING: Spectral shift pattern
        TMap<float, float> SpectralShifts = AnalyzeSpectralPattern(Pattern);
        if (SpectralShifts.Num() > 0)
        {
            Rule.Type = ETransformationType::ColorMapping;
            
            // Convert wavelength shifts to color mapping
            for (const auto& Shift : SpectralShifts)
            {
                int32 FromColor = WavelengthToColor(Shift.Key);
                int32 ToColor = WavelengthToColor(Shift.Value);
                Rule.ColorMap.Add(FromColor, ToColor);
            }
            
            Rule.Confidence = CalculateSpectralCoherence(Pattern);
            return Rule;
        }
        
        // 5. PATTERN FILL: Oscillating activation
        if (DetectOscillation(Pattern))
        {
            Rule.Type = ETransformationType::PatternFill;
            
            FARCGrid PatternGrid = ExtractOscillationPattern(Pattern);
            
            Rule.CustomTransform = [PatternGrid](const FARCGrid& Input) -> FARCGrid
            {
                return FillWithPattern(Input, PatternGrid);
            };
            
            Rule.Confidence = 0.7f;
            return Rule;
        }
        
        // 6. CONNECTIVITY: Bridge formation between clusters
        if (DetectBridgeFormation(Pattern))
        {
            Rule.Type = ETransformationType::Connectivity;
            
            Rule.CustomTransform = [](const FARCGrid& Input) -> FARCGrid
            {
                return ConnectComponents(Input);
            };
            
            Rule.Confidence = 0.75f;
            return Rule;
        }
        
        // Default: No clear pattern
        Rule.Type = ETransformationType::None;
        Rule.Confidence = 0.0f;
        
        return Rule;
    }
    
private:
    float DetermineRotationAngle(const FClusterPattern& Pattern)
    {
        // Analyze positions over time to determine rotation
        // Using moment of inertia tensor
        
        FMatrix InertiaTensor = FMatrix::Identity;
        float TotalMass = 0;
        
        FVector3f CenterOfMass = CalculateCenterOfMass(Pattern.Positions);
        
        for (int32 i = 0; i < Pattern.Positions.Num(); i++)
        {
            FVector3f R = Pattern.Positions[i] - CenterOfMass;
            float Mass = Pattern.Activations[i];  // Use activation as mass proxy
            
            InertiaTensor.M[0][0] += Mass * (R.Y * R.Y + R.Z * R.Z);
            InertiaTensor.M[1][1] += Mass * (R.X * R.X + R.Z * R.Z);
            InertiaTensor.M[2][2] += Mass * (R.X * R.X + R.Y * R.Y);
            
            InertiaTensor.M[0][1] -= Mass * R.X * R.Y;
            InertiaTensor.M[0][2] -= Mass * R.X * R.Z;
            InertiaTensor.M[1][2] -= Mass * R.Y * R.Z;
            
            TotalMass += Mass;
        }
        
        // Symmetrize
        InertiaTensor.M[1][0] = InertiaTensor.M[0][1];
        InertiaTensor.M[2][0] = InertiaTensor.M[0][2];
        InertiaTensor.M[2][1] = InertiaTensor.M[1][2];
        
        // Eigenvalue decomposition to find principal axes
        FVector Eigenvalues;
        FMatrix Eigenvectors;
        InertiaTensor.GetEigenVectors(Eigenvectors, Eigenvalues);
        
        // Rotation angle from principal axis alignment
        FVector PrincipalAxis = Eigenvectors.GetColumn(0);
        float Angle = FMath::Atan2(PrincipalAxis.Y, PrincipalAxis.X);
        
        // Quantize to 90-degree increments for ARC grids
        Angle = FMath::RoundToFloat(Angle / (PI / 2)) * (PI / 2);
        
        return Angle;
    }
    
    TMap<float, float> AnalyzeSpectralPattern(const FClusterPattern& Pattern)
    {
        TMap<float, float> Shifts;
        
        // Group neurons by input wavelength
        TMap<float, TArray<float>> WavelengthGroups;
        
        for (int32 i = 0; i < Pattern.Wavelengths.Num(); i++)
        {
            float InputWavelength = Pattern.Wavelengths[i];
            
            // Quantize to color bins
            float BinnedWavelength = FMath::RoundToFloat(InputWavelength / 50e-9) * 50e-9;
            
            if (!WavelengthGroups.Contains(BinnedWavelength))
            {
                WavelengthGroups.Add(BinnedWavelength, TArray<float>());
            }
            
            // Track output wavelength (based on position in pattern)
            float OutputWavelength = CalculateOutputWavelength(Pattern.Positions[i], Pattern);
            WavelengthGroups[BinnedWavelength].Add(OutputWavelength);
        }
        
        // Find consistent mappings
        for (const auto& Group : WavelengthGroups)
        {
            if (Group.Value.Num() > 5)  // Minimum samples
            {
                // Calculate mode (most common output)
                float ModeWavelength = CalculateMode(Group.Value);
                
                if (FMath::Abs(ModeWavelength - Group.Key) > 10e-9)  // Significant shift
                {
                    Shifts.Add(Group.Key, ModeWavelength);
                }
            }
        }
        
        return Shifts;
    }
    
    bool DetectOscillation(const FClusterPattern& Pattern)
    {
        // Fourier analysis of activation patterns
        if (Pattern.Activations.Num() < 16)
            return false;
        
        // Simple FFT to detect periodicity
        TArray<FComplex> FFTData;
        FFTData.Reserve(Pattern.Activations.Num());
        
        for (float Activation : Pattern.Activations)
        {
            FFTData.Add(FComplex(Activation, 0));
        }
        
        // Compute FFT (simplified - in production use FFTW or similar)
        TArray<FComplex> Spectrum = ComputeFFT(FFTData);
        
        // Look for dominant frequency
        float MaxMagnitude = 0;
        int32 DominantFreq = 0;
        
        for (int32 i = 1; i < Spectrum.Num() / 2; i++)
        {
            float Magnitude = Spectrum[i].Magnitude();
            if (Magnitude > MaxMagnitude)
            {
                MaxMagnitude = Magnitude;
                DominantFreq = i;
            }
        }
        
        // Oscillation detected if strong periodic component
        float PowerRatio = MaxMagnitude / Spectrum[0].Magnitude();
        return PowerRatio > 0.5f && DominantFreq > 0;
    }
    
    bool DetectBridgeFormation(const FClusterPattern& Pattern)
    {
        // Analyze connectivity changes
        // Look for neurons forming lines between previously disconnected regions
        
        // Compute minimum spanning tree
        TArray<FEdge> MST = ComputeMST(Pattern.Positions);
        
        // Check for long edges (bridges)
        float AvgEdgeLength = 0;
        float MaxEdgeLength = 0;
        
        for (const FEdge& Edge : MST)
        {
            float Length = FVector3f::Dist(
                Pattern.Positions[Edge.Start],
                Pattern.Positions[Edge.End]
            );
            
            AvgEdgeLength += Length;
            MaxEdgeLength = FMath::Max(MaxEdgeLength, Length);
        }
        
        AvgEdgeLength /= MST.Num();
        
        // Bridge detected if some edges are much longer than average
        return (MaxEdgeLength > AvgEdgeLength * 3.0f);
    }
};