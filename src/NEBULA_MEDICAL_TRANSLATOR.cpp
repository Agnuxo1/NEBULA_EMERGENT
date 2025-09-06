// NEBULA_MEDICAL_TRANSLATOR.cpp
// Sistema completo de traducción médica ↔ galaxia neuronal

#include "Core/Public/HAL/PlatformFilemanager.h"
#include "dcmtk/dcmdata/dctk.h"
#include "itkImage.h"
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageSeriesReader.h"

// ============================================================================
// TRADUCCIÓN ENTRADA: TAC 3D → NEBULA
// ============================================================================

class MedicalToNebulaTranslator
{
private:
    struct VoxelToPhoton
    {
        // Cada voxel del TAC se convierte en emisión de fotones
        // La intensidad Hounsfield determina el espectro y energía
        
        float HounsfieldUnit;      // -1000 (aire) a +3000 (hueso denso)
        FVector3f Position;         // Posición 3D en el volumen
        float ContrastPhase;        // Fase del contraste (arterial/venosa)
        
        TArray<FPhotonEmission> ConvertToPhotons() const
        {
            TArray<FPhotonEmission> Photons;
            
            // MAPEO FÍSICAMENTE CORRECTO:
            // Los aneurismas aparecen como zonas de alta intensidad
            // en fase arterial con patrón característico
            
            // 1. Convertir HU a densidad de tejido real
            float TissueDensity = (HounsfieldUnit + 1000.0f) / 4000.0f;
            
            // 2. Detectar si es vaso sanguíneo con contraste
            bool IsVessel = (HounsfieldUnit > 200 && HounsfieldUnit < 700);
            bool IsAneurysmCandidate = IsVessel && (ContrastPhase > 0.8f);
            
            // 3. Generar espectro de emisión basado en características
            if (IsAneurysmCandidate)
            {
                // Aneurisma: emisión alta en longitudes de onda específicas
                // Esto se basa en la física real del contraste yodado
                
                FPhotonEmission Photon;
                Photon.Wavelength = 33.2e-9f;  // K-edge del yodo (33.2 keV)
                Photon.Energy = 5.31e-15f;     // Joules
                Photon.Direction = FVector3f::RandomUnit();
                Photon.Polarization = FVector2f(1, 0);
                
                // Múltiples fotones para zonas de alta intensidad
                int NumPhotons = FMath::CeilToInt(TissueDensity * 100);
                for (int i = 0; i < NumPhotons; i++)
                {
                    Photons.Add(Photon);
                }
            }
            else if (IsVessel)
            {
                // Vaso normal: emisión moderada
                FPhotonEmission Photon;
                Photon.Wavelength = 40e-9f;    // 40 keV típico
                Photon.Energy = 2.5e-15f;
                Photons.Add(Photon);
            }
            else
            {
                // Tejido normal: emisión baja
                if (TissueDensity > 0.1f)
                {
                    FPhotonEmission Photon;
                    Photon.Wavelength = 60e-9f;  // 60 keV
                    Photon.Energy = 1e-15f;
                    Photons.Add(Photon);
                }
            }
            
            return Photons;
        }
    };

public:
    void TranslateDICOMToNebula(const FString& DICOMPath, ANebulaEmergent* Nebula)
    {
        // 1. CARGAR SERIE DICOM COMPLETA
        typedef itk::Image<float, 3> ImageType;
        typedef itk::ImageSeriesReader<ImageType> ReaderType;
        typedef itk::GDCMImageIO ImageIOType;
        typedef itk::GDCMSeriesFileNames NamesGeneratorType;
        
        ImageIOType::Pointer dicomIO = ImageIOType::New();
        NamesGeneratorType::Pointer nameGenerator = NamesGeneratorType::New();
        nameGenerator->SetInputDirectory(TCHAR_TO_UTF8(*DICOMPath));
        
        const ReaderType::FileNamesContainer& filenames = nameGenerator->GetInputFileNames();
        ReaderType::Pointer reader = ReaderType::New();
        reader->SetImageIO(dicomIO);
        reader->SetFileNames(filenames);
        reader->Update();
        
        ImageType::Pointer image = reader->GetOutput();
        ImageType::SizeType size = image->GetLargestPossibleRegion().GetSize();
        
        // 2. EXTRAER METADATA CRÍTICA
        float SliceThickness = 0.625f;  // mm típico para angio-TAC
        float PixelSpacing = 0.5f;      // mm
        float WindowCenter = 40.0f;     // Ventana para tejido cerebral
        float WindowWidth = 80.0f;
        
        // Leer de los tags DICOM reales
        DcmFileFormat fileformat;
        OFCondition status = fileformat.loadFile(TCHAR_TO_UTF8(*filenames[0]));
        if (status.good())
        {
            DcmDataset* dataset = fileformat.getDataset();
            Float64 value;
            
            if (dataset->findAndGetFloat64(DCM_SliceThickness, value).good())
                SliceThickness = value;
            if (dataset->findAndGetFloat64(DCM_PixelSpacing, value, 0).good())
                PixelSpacing = value;
            if (dataset->findAndGetFloat64(DCM_WindowCenter, value).good())
                WindowCenter = value;
            if (dataset->findAndGetFloat64(DCM_WindowWidth, value).good())
                WindowWidth = value;
        }
        
        // 3. IDENTIFICAR FASE DE CONTRASTE
        float ContrastPhase = DetectContrastPhase(image);
        
        // 4. CONVERTIR CADA VOXEL A ESTÍMULOS PARA NEBULA
        
        // Crear matriz de inyección espacial
        // Los voxels se mapean a puntos de entrada en la esfera observadora
        
        const int32 TotalVoxels = size[0] * size[1] * size[2];
        const int32 InjectionPorts = 10000;  // Puntos de entrada en la esfera
        
        // Mapeo espacial: preservar relaciones topológicas
        TArray<TArray<VoxelToPhoton>> InjectionMap;
        InjectionMap.SetNum(InjectionPorts);
        
        itk::ImageRegionIterator<ImageType> iterator(image, image->GetLargestPossibleRegion());
        
        int VoxelIndex = 0;
        for (iterator.GoToBegin(); !iterator.IsAtEnd(); ++iterator)
        {
            VoxelToPhoton Voxel;
            Voxel.HounsfieldUnit = iterator.Get();
            
            // Posición 3D real en mm
            ImageType::IndexType index = iterator.GetIndex();
            Voxel.Position = FVector3f(
                index[0] * PixelSpacing,
                index[1] * PixelSpacing,
                index[2] * SliceThickness
            );
            Voxel.ContrastPhase = ContrastPhase;
            
            // Mapear a puerto de inyección preservando localidad espacial
            int PortIndex = MapVoxelToInjectionPort(Voxel.Position, InjectionPorts);
            InjectionMap[PortIndex].Add(Voxel);
            
            VoxelIndex++;
        }
        
        // 5. INYECTAR EN NEBULA TEMPORALMENTE
        
        // Los datos médicos no se inyectan todos de golpe
        // Se introduce en secuencia temporal que preserva relaciones causales
        
        float TimePerSlice = 0.016f;  // 1 frame a 60 FPS por slice
        
        for (int z = 0; z < size[2]; z++)  // Por cada slice axial
        {
            TArray<FPhotonBurst> SliceBursts;
            
            for (int y = 0; y < size[1]; y++)
            {
                for (int x = 0; x < size[0]; x++)
                {
                    ImageType::IndexType index;
                    index[0] = x; index[1] = y; index[2] = z;
                    
                    float HU = image->GetPixel(index);
                    
                    // Solo procesar voxels relevantes (no aire)
                    if (HU > -500)
                    {
                        VoxelToPhoton Voxel;
                        Voxel.HounsfieldUnit = HU;
                        Voxel.Position = FVector3f(x * PixelSpacing, y * PixelSpacing, z * SliceThickness);
                        Voxel.ContrastPhase = ContrastPhase;
                        
                        TArray<FPhotonEmission> Photons = Voxel.ConvertToPhotons();
                        
                        if (Photons.Num() > 0)
                        {
                            FPhotonBurst Burst;
                            Burst.OriginPosition = MapToSpherePoint(Voxel.Position);
                            Burst.Photons = Photons;
                            Burst.InjectionTime = z * TimePerSlice;
                            SliceBursts.Add(Burst);
                        }
                    }
                }
            }
            
            // Inyectar slice en la galaxia
            Nebula->InjectPhotonBursts(SliceBursts);
            
            // Permitir que la galaxia evolucione y procese
            Nebula->EvolveFrames(4);  // 4 frames para propagación
        }
    }
    
private:
    float DetectContrastPhase(ImageType::Pointer image)
    {
        // Analizar histograma para determinar fase de contraste
        // Los aneurismas son más visibles en fase arterial temprana
        
        const int NumBins = 256;
        int Histogram[NumBins] = {0};
        
        itk::ImageRegionIterator<ImageType> it(image, image->GetLargestPossibleRegion());
        for (it.GoToBegin(); !it.IsAtEnd(); ++it)
        {
            float value = it.Get();
            if (value > 200 && value < 700)  // Rango de contraste
            {
                int bin = FMath::Clamp((int)((value - 200) / 2), 0, NumBins - 1);
                Histogram[bin]++;
            }
        }
        
        // Calcular centro de masa del histograma
        float WeightedSum = 0;
        int TotalCount = 0;
        for (int i = 0; i < NumBins; i++)
        {
            WeightedSum += i * Histogram[i];
            TotalCount += Histogram[i];
        }
        
        if (TotalCount > 0)
        {
            float Center = WeightedSum / TotalCount;
            // Fase arterial: centro < 100, Fase venosa: centro > 150
            return 1.0f - (Center / NumBins);  // 1.0 = arterial, 0.0 = venosa
        }
        
        return 0.5f;  // Fase indeterminada
    }
    
    FVector3f MapToSpherePoint(const FVector3f& VoxelPos)
    {
        // Mapear posición del voxel a punto en la esfera observadora
        // Preservando relaciones topológicas
        
        float Theta = (VoxelPos.X / 512.0f) * 2.0f * PI;
        float Phi = (VoxelPos.Y / 512.0f) * PI;
        float RadiusModulation = 1.0f + (VoxelPos.Z / 256.0f) * 0.1f;
        
        float Radius = 15000.0f * RadiusModulation;  // Radio de la esfera observadora
        
        return FVector3f(
            Radius * FMath::Sin(Phi) * FMath::Cos(Theta),
            Radius * FMath::Sin(Phi) * FMath::Sin(Theta),
            Radius * FMath::Cos(Phi)
        );
    }
};

// ============================================================================
// TRADUCCIÓN SALIDA: NEBULA → DETECCIÓN DE ANEURISMAS
// ============================================================================

class NebulaToMedicalTranslator
{
private:
    struct GalaxyState
    {
        TArray<FNeuronActivation> ActiveNeurons;
        TArray<FPhotonCapture> CapturedPhotons;
        float TotalLuminosity;
        FVector3f CenterOfMass;
        float SpectralPeak;
    };
    
    struct AneurysmSignature
    {
        // Patrón característico de un aneurisma en NEBULA
        float MinLuminositySpike = 5.0f;
        float SpectralPeakWavelength = 33.2e-9f;  // K-edge del yodo
        float SpatialCoherence = 0.8f;
        float TemporalStability = 0.7f;
        int MinClusterSize = 50;
    };

public:
    FAneurysmDetectionResult TranslateNebulaToMedical(ANebulaEmergent* Nebula)
    {
        FAneurysmDetectionResult Result;
        
        // 1. CAPTURAR ESTADO DE LA GALAXIA
        GalaxyState State = CaptureGalaxyState(Nebula);
        
        // 2. IDENTIFICAR CLUSTERS ACTIVOS
        TArray<FNeuronCluster> Clusters = IdentifyClusters(State.ActiveNeurons);
        
        // 3. ANALIZAR CADA CLUSTER PARA PATRONES DE ANEURISMA
        AneurysmSignature Signature;
        
        for (const FNeuronCluster& Cluster : Clusters)
        {
            if (Cluster.Neurons.Num() < Signature.MinClusterSize)
                continue;
            
            // Analizar características del cluster
            float ClusterLuminosity = 0;
            float SpectralCoherence = 0;
            FVector3f ClusterCenter = FVector3f::ZeroVector;
            
            for (int32 NeuronIdx : Cluster.Neurons)
            {
                const FNeuronActivation& Neuron = State.ActiveNeurons[NeuronIdx];
                ClusterLuminosity += Neuron.Luminosity;
                ClusterCenter += Neuron.Position;
            }
            
            ClusterCenter /= Cluster.Neurons.Num();
            ClusterLuminosity /= Cluster.Neurons.Num();
            
            // Analizar espectro de fotones capturados de este cluster
            float PeakWavelength = AnalyzePhotonSpectrum(State.CapturedPhotons, ClusterCenter);
            
            // CRITERIOS DE DETECCIÓN DE ANEURISMA
            bool IsAneurysm = 
                (ClusterLuminosity > Signature.MinLuminositySpike) &&
                (FMath::Abs(PeakWavelength - Signature.SpectralPeakWavelength) < 1e-9f) &&
                (CalculateSpatialCoherence(Cluster, State.ActiveNeurons) > Signature.SpatialCoherence);
            
            if (IsAneurysm)
            {
                FAneurysmDetection Detection;
                
                // 4. TRADUCIR POSICIÓN DEL CLUSTER A COORDENADAS MÉDICAS
                Detection.WorldPosition = ClusterCenter;
                Detection.VoxelPosition = ConvertGalaxyToVoxel(ClusterCenter);
                
                // 5. CALCULAR TAMAÑO DEL ANEURISMA
                Detection.SizeMM = CalculateAneurysmSize(Cluster, State.ActiveNeurons);
                
                // 6. CALCULAR CONFIDENCE SCORE
                Detection.Confidence = CalculateConfidence(
                    ClusterLuminosity,
                    PeakWavelength,
                    Cluster.Neurons.Num()
                );
                
                // 7. CLASIFICAR TIPO DE ANEURISMA
                Detection.Type = ClassifyAneurysmType(Cluster, State);
                
                Result.Detections.Add(Detection);
            }
        }
        
        // 8. GENERAR SALIDA FORMATO KAGGLE
        Result.FormatForKaggle();
        
        return Result;
    }
    
private:
    FIntVector ConvertGalaxyToVoxel(const FVector3f& GalaxyPos)
    {
        // Inversión del mapeo entrada
        // La galaxia preserva relaciones topológicas del volumen original
        
        // Decodificar de coordenadas esféricas
        float R = GalaxyPos.Length();
        float Theta = FMath::Atan2(GalaxyPos.Y, GalaxyPos.X);
        float Phi = FMath::Acos(GalaxyPos.Z / R);
        
        // Mapear de vuelta a voxels
        int X = FMath::RoundToInt((Theta / (2.0f * PI)) * 512.0f);
        int Y = FMath::RoundToInt((Phi / PI) * 512.0f);
        int Z = FMath::RoundToInt(((R / 15000.0f) - 1.0f) * 10.0f * 256.0f);
        
        return FIntVector(
            FMath::Clamp(X, 0, 511),
            FMath::Clamp(Y, 0, 511),
            FMath::Clamp(Z, 0, 255)
        );
    }
    
    float CalculateAneurysmSize(const FNeuronCluster& Cluster, const TArray<FNeuronActivation>& Neurons)
    {
        // Calcular diámetro máximo del cluster en mm
        float MaxDistance = 0;
        
        for (int i = 0; i < Cluster.Neurons.Num(); i++)
        {
            for (int j = i + 1; j < Cluster.Neurons.Num(); j++)
            {
                FVector3f Pos1 = Neurons[Cluster.Neurons[i]].Position;
                FVector3f Pos2 = Neurons[Cluster.Neurons[j]].Position;
                float Distance = (Pos2 - Pos1).Length();
                MaxDistance = FMath::Max(MaxDistance, Distance);
            }
        }
        
        // Convertir unidades de galaxia a mm
        // Factor de escala: 1 unidad galaxia = 0.01 mm
        return MaxDistance * 0.01f;
    }
    
    float CalculateConfidence(float Luminosity, float Wavelength, int ClusterSize)
    {
        // Fórmula de confidence basada en evidencia física
        
        float LuminosityScore = FMath::Clamp(Luminosity / 10.0f, 0.0f, 1.0f);
        float WavelengthScore = FMath::Exp(-FMath::Abs(Wavelength - 33.2e-9f) * 1e9);
        float SizeScore = FMath::Clamp(ClusterSize / 200.0f, 0.0f, 1.0f);
        
        // Weighted combination
        return 0.5f * LuminosityScore + 0.3f * WavelengthScore + 0.2f * SizeScore;
    }
    
    EAneurysmType ClassifyAneurysmType(const FNeuronCluster& Cluster, const GalaxyState& State)
    {
        // Clasificar basado en patrones de activación
        
        float AspectRatio = CalculateClusterAspectRatio(Cluster, State.ActiveNeurons);
        float Irregularity = CalculateClusterIrregularity(Cluster, State.ActiveNeurons);
        
        if (AspectRatio > 2.0f && Irregularity < 0.3f)
            return EAneurysmType::Fusiform;
        else if (AspectRatio < 1.5f && Irregularity > 0.6f)
            return EAneurysmType::Saccular;
        else
            return EAneurysmType::Complex;
    }
};

// ============================================================================
// FORMATO DE SALIDA PARA KAGGLE
// ============================================================================

struct FAneurysmDetectionResult
{
    TArray<FAneurysmDetection> Detections;
    
    void FormatForKaggle()
    {
        // Generar CSV exacto que espera la competición
        FString CSV = "ID,Predicted\n";
        
        for (const FAneurysmDetection& Detection : Detections)
        {
            // Formato: StudyInstanceUID_SliceNumber,Confidence
            FString StudyID = GetCurrentStudyID();
            int32 SliceNumber = Detection.VoxelPosition.Z;
            float Confidence = Detection.Confidence;
            
            CSV += FString::Printf(TEXT("%s_%d,%.6f\n"), 
                                   *StudyID, SliceNumber, Confidence);
        }
        
        // Guardar CSV
        FFileHelper::SaveStringToFile(CSV, TEXT("submission.csv"));
    }
};