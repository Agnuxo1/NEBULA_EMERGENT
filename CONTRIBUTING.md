# Contributing to NEBULA EMERGENT

Thank you for your interest in contributing to NEBULA EMERGENT! This document provides guidelines for contributing to this physics-based neural architecture project.

## Code of Conduct

This project adheres to scientific rigor and professional standards. All contributions must:

- Be based on established physics principles
- Include proper validation and testing
- Maintain code quality and documentation standards
- Respect intellectual property and attribution

## How to Contribute

### Reporting Issues

1. **Search existing issues** first to avoid duplicates
2. **Use the issue template** when creating new issues
3. **Provide detailed information**:
   - Operating system and compiler version
   - Steps to reproduce the problem
   - Expected vs actual behavior
   - Relevant code snippets or error messages

### Submitting Changes

1. **Fork the repository** and create a feature branch
2. **Follow coding standards**:
   - C++17 standard compliance
   - Consistent naming conventions
   - Comprehensive comments and documentation
   - Physics validation for any new algorithms

3. **Include tests** for new functionality
4. **Update documentation** as needed
5. **Submit a pull request** with detailed description

### Development Setup

```bash
# Clone your fork
git clone https://github.com/yourusername/NEBULA_EMERGENT.git
cd NEBULA_EMERGENT

# Build the project
make all

# Run tests
make test

# Run benchmarks
make benchmark
```

## Coding Standards

### C++ Guidelines

- Use C++17 features appropriately
- Follow RAII principles
- Prefer const correctness
- Use meaningful variable and function names
- Include proper error handling

### Physics Implementation

- All physical constants must use exact values
- Include units in comments
- Validate against established physics literature
- Document mathematical derivations

### Example Code Style

```cpp
// NEBULA EMERGENT - Physics-based neural computation
// Author: Your Name

#include <vector>
#include <cmath>

class PhysicsComponent {
private:
    // Speed of light in m/s (exact value)
    static constexpr float SPEED_OF_LIGHT = 299792458.0f;
    
public:
    /**
     * Calculate electromagnetic propagation
     * @param position Initial position vector
     * @param direction Normalized direction vector
     * @param deltaTime Time step in seconds
     * @return New position after propagation
     */
    Vector3 propagateElectromagnetic(const Vector3& position, 
                                   const Vector3& direction, 
                                   float deltaTime) const {
        // Apply special relativity constraints
        return position + direction * (SPEED_OF_LIGHT * deltaTime);
    }
};
```

## Testing Requirements

### Unit Tests

All new functionality must include unit tests:

```cpp
void testElectromagneticPropagation() {
    PhysicsComponent physics;
    Vector3 start(0, 0, 0);
    Vector3 direction(1, 0, 0);
    float deltaTime = 1e-9f; // 1 nanosecond
    
    Vector3 result = physics.propagateElectromagnetic(start, direction, deltaTime);
    
    // Should travel ~30cm in 1ns
    assert(std::abs(result.x - 0.299792458f) < 1e-6f);
}
```

### Physics Validation

New physics implementations must be validated:

1. **Literature Review**: Cite relevant physics papers
2. **Numerical Validation**: Compare with analytical solutions
3. **Conservation Laws**: Verify energy/momentum conservation
4. **Dimensional Analysis**: Ensure correct units

### Performance Testing

Performance-critical code should include benchmarks:

```cpp
void benchmarkNBodySimulation() {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Run simulation
    galaxy.evolveFrame(0.016f);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "N-body simulation: " << duration.count() << " μs" << std::endl;
}
```

## Documentation Standards

### Code Documentation

Use Doxygen-style comments:

```cpp
/**
 * @brief Evolve neural galaxy for one time step
 * @param deltaTime Time step in seconds
 * @details Performs N-body gravitational simulation, electromagnetic
 *          propagation, and neural connectivity updates
 * @note Maintains energy conservation to >99.9% accuracy
 */
void evolveFrame(float deltaTime);
```

### README Updates

When adding new features:

1. Update the feature list
2. Add usage examples
3. Update performance metrics
4. Include validation results

## Scientific Rigor

### Physics Accuracy

- Use exact physical constants
- Implement established equations correctly
- Validate against known solutions
- Document assumptions and approximations

### Experimental Validation

- Include test results in pull requests
- Provide performance benchmarks
- Document parameter sensitivity
- Compare with theoretical predictions

### Literature References

Cite relevant scientific literature:

```cpp
/**
 * Implementation of Wien's displacement law
 * Reference: Wien, W. (1893). "Eine neue Beziehung der Strahlung 
 * schwarzer Körper zum zweiten Hauptsatz der Wärmetheorie"
 */
float calculatePeakWavelength(float temperature) {
    const float WIEN_CONSTANT = 2.897771955e-3f; // m⋅K
    return WIEN_CONSTANT / temperature;
}
```

## Review Process

### Pull Request Review

All submissions undergo review for:

1. **Code Quality**: Style, structure, and maintainability
2. **Physics Accuracy**: Correct implementation of physical laws
3. **Performance**: Computational efficiency and scalability
4. **Testing**: Adequate test coverage and validation
5. **Documentation**: Clear and comprehensive documentation

### Acceptance Criteria

Pull requests must:

- Pass all existing tests
- Include tests for new functionality
- Maintain or improve performance
- Follow coding standards
- Include proper documentation
- Demonstrate physics validation

## Community Guidelines

### Communication

- Be respectful and professional
- Focus on technical merit
- Provide constructive feedback
- Share knowledge and expertise

### Collaboration

- Credit all contributors appropriately
- Share ideas and improvements
- Help others learn and contribute
- Maintain scientific integrity

## Getting Help

### Resources

- **Documentation**: Check the `docs/` directory
- **Examples**: Review code in `examples/` directory
- **Tests**: Examine test cases for usage patterns
- **Issues**: Search existing issues for solutions

### Contact

- **GitHub Issues**: For bugs and feature requests
- **Discussions**: For general questions and ideas
- **Email**: francisco.angulo@nebula-research.org for direct contact

## Recognition

Contributors will be recognized in:

- **README.md**: Contributor list
- **CHANGELOG.md**: Feature attribution
- **Scientific Papers**: Co-authorship for significant contributions
- **Presentations**: Conference acknowledgments

## License

By contributing to NEBULA EMERGENT, you agree that your contributions will be licensed under the MIT License.

---

Thank you for contributing to the advancement of physics-based neural architectures!

**NEBULA Team**  
Francisco Angulo de Lafuente  
NEBULA Research Initiative

