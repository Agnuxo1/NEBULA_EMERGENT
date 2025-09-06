// DiversityMaintenance.cpp
// Mechanisms to prevent premature convergence

class FDiversityController
{
private:
    // System temperature (controls exploration vs exploitation)
    float SystemTemperature = 1000.0f;  // Starts hot (exploration)
    float CoolingRate = 0.995f;         // Gradual cooling
    float MinTemperature = 10.0f;       // Never fully frozen
    
    // Lateral inhibition parameters
    float InhibitionRadius = 500.0f;
    float InhibitionStrength = 0.5f;
    
public:
    void UpdateSystemDynamics(
        TArray<FNeuronState>& Neurons,
        float DeltaTime,
        int32 IterationCount)
    {
        // 1. SIMULATED ANNEALING: Temperature-based exploration
        ApplyThermalNoise(Neurons, DeltaTime);
        
        // 2. LATERAL INHIBITION: Bright clusters suppress nearby formation
        ApplyLateralInhibition(Neurons);
        
        // 3. DIVERSITY PRESSURE: Penalize overly large clusters
        ApplyDiversityPressure(Neurons);
        
        // 4. PERIODIC PERTURBATION: Shake the system occasionally
        if (IterationCount % 100 == 0)
        {
            ApplyRandomPerturbation(Neurons);
        }
        
        // Cool down gradually
        SystemTemperature *= CoolingRate;
        SystemTemperature = FMath::Max(SystemTemperature, MinTemperature);
    }
    
private:
    void ApplyThermalNoise(TArray<FNeuronState>& Neurons, float DeltaTime)
    {
        for (FNeuronState& Neuron : Neurons)
        {
            // Brownian motion proportional to temperature
            FVector RandomForce = FVector(
                FMath::RandRange(-1.0f, 1.0f),
                FMath::RandRange(-1.0f, 1.0f),
                FMath::RandRange(-1.0f, 1.0f)
            );
            
            float NoiseMagnitude = FMath::Sqrt(SystemTemperature) * 0.01f;
            Neuron.Velocity += RandomForce * NoiseMagnitude * DeltaTime;
            
            // Stochastic luminosity fluctuations
            float LuminosityNoise = FMath::RandRange(-0.1f, 0.1f) * SystemTemperature / 1000.0f;
            Neuron.Luminosity *= (1.0f + LuminosityNoise);
            Neuron.Luminosity = FMath::Clamp(Neuron.Luminosity, 0.1f, 100.0f);
        }
    }
    
    void ApplyLateralInhibition(TArray<FNeuronState>& Neurons)
    {
        // Bright neurons suppress nearby neurons (like in retina)
        TArray<float> InhibitionField;
        InhibitionField.SetNum(Neurons.Num());
        
        // Calculate inhibition field
        for (int32 i = 0; i < Neurons.Num(); i++)
        {
            if (Neurons[i].Luminosity > 5.0f)  // Only bright neurons inhibit
            {
                for (int32 j = 0; j < Neurons.Num(); j++)
                {
                    if (i != j)
                    {
                        float Distance = FVector::Dist(
                            Neurons[i].Position,
                            Neurons[j].Position
                        );
                        
                        if (Distance < InhibitionRadius)
                        {
                            float Inhibition = InhibitionStrength * 
                                               Neurons[i].Luminosity * 
                                               FMath::Exp(-Distance / InhibitionRadius);
                            
                            InhibitionField[j] += Inhibition;
                        }
                    }
                }
            }
        }
        
        // Apply inhibition
        for (int32 i = 0; i < Neurons.Num(); i++)
        {
            Neurons[i].Luminosity /= (1.0f + InhibitionField[i]);
            
            // Inhibited neurons are pushed away
            if (InhibitionField[i] > 0.1f)
            {
                FVector RepulsionDirection = FVector(
                    FMath::RandRange(-1.0f, 1.0f),
                    FMath::RandRange(-1.0f, 1.0f),
                    FMath::RandRange(-1.0f, 1.0f)
                ).GetSafeNormal();
                
                Neurons[i].Velocity += RepulsionDirection * InhibitionField[i] * 10.0f;
            }
        }
    }
    
    void ApplyDiversityPressure(TArray<FNeuronState>& Neurons)
    {
        // Find clusters
        TArray<FNeuronCluster> Clusters = IdentifyClusters(Neurons);
        
        for (const FNeuronCluster& Cluster : Clusters)
        {
            // Penalize clusters that are too large (monopolizing resources)
            if (Cluster.NeuronIDs.Num() > Neurons.Num() / 10)  // >10% of all neurons
            {
                for (int32 ID : Cluster.NeuronIDs)
                {
                    // Reduce luminosity of overgrown clusters
                    Neurons[ID].Luminosity *= 0.95f;
                    
                    // Increase fission probability
                    Neurons[ID].Energy *= 0.9f;
                }
            }
        }
    }
    
    void ApplyRandomPerturbation(TArray<FNeuronState>& Neurons)
    {
        // Occasionally "kick" the system to escape local minima
        
        int32 NumToPerturb = Neurons.Num() / 100;  // Perturb 1% of neurons
        
        for (int32 i = 0; i < NumToPerturb; i++)
        {
            int32 RandomIndex = FMath::RandRange(0, Neurons.Num() - 1);
            
            // Random velocity boost
            FVector RandomKick = FVector(
                FMath::RandRange(-100.0f, 100.0f),
                FMath::RandRange(-100.0f, 100.0f),
                FMath::RandRange(-100.0f, 100.0f)
            );
            
            Neurons[RandomIndex].Velocity += RandomKick;
            
            // Random luminosity spike (mutation)
            if (FMath::RandRange(0.0f, 1.0f) < 0.1f)
            {
                Neurons[RandomIndex].Luminosity *= FMath::RandRange(2.0f, 5.0f);
            }
        }
    }
};

// Integration into main evolution loop
void ANebulaEmergent::EvolveFrame(float DeltaTime)
{
    static FValidityOracle Oracle;
    static FDiversityController Diversity;
    static int32 IterationCount = 0;
    
    // 1. Evaluate current patterns against training data
    TArray<FNeuronCluster> Clusters = IdentifyActiveClusters();
    
    for (FNeuronCluster& Cluster : Clusters)
    {
        float Validity = Oracle.EvaluatePatternValidity(Cluster, CurrentTrainingExamples);
        
        // Update luminosity based on validity
        for (int32 NeuronID : Cluster.NeuronIDs)
        {
            Oracle.UpdateNeuronLuminosity(Neurons[NeuronID], Validity, DeltaTime);
        }
    }
    
    // 2. Physics evolution with luminosity-based attraction
    DispatchNeuronEvolution(DeltaTime);
    
    // 3. Maintain diversity to avoid local maxima
    Diversity.UpdateSystemDynamics(Neurons, DeltaTime, IterationCount);
    
    IterationCount++;
}