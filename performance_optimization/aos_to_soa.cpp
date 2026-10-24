#include <cstddef>
#include<iostream>
#include <stdexcept>
#include<vector>
#include<chrono>
#include<random>
#include<string>
#include<algorithm>


// Current implementation - Array of Structures
struct Particle {
    double x, y, z;      // Position
    double vx, vy, vz;   // Velocity
    double mass;
    int id;
};

class ParticleSystemAoS {
private:
    std::vector<Particle> particles;
    
public:
    void resize(size_t count) {
        particles.resize(count);
    }
    
    void initializeParticles() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> pos(-100.0, 100.0);
        std::uniform_real_distribution<double> vel(-10.0, 10.0);
        std::uniform_real_distribution<double> mass(0.1, 10.0);
        
        for (size_t i = 0; i < particles.size(); ++i) {
            particles[i] = {pos(gen), pos(gen), pos(gen),
                           vel(gen), vel(gen), vel(gen),
                           mass(gen), static_cast<int>(i)};
        }
    }
    
    double calculateTotalKineticEnergy() const {
        double totalEnergy = 0.0;
        for (const auto& p : particles) {
            double vel2 = p.vx * p.vx + p.vy * p.vy + p.vz * p.vz;
            totalEnergy += 0.5 * p.mass * vel2;
        }
        return totalEnergy;
    }
};

////////////////////////////
class ParticleSystemSoA{
    private:
        std::vector<double> pos_x;
        std::vector<double> pos_y;
        std::vector<double> pos_z;
        std::vector<double>  vx;
        std::vector<double>  vy;
        std::vector<double>  vz;
        std::vector<double>  mass;
        std::vector<int> particles_id;
        const std::vector<std::string> fieldNameBuf{
            "pos_x",
                "pos_y",
                "pos_z",
                "vx",
                "vy",
                "vz",
                "mass",
                "particle_id"
        };


    public:
        void resize(std::size_t count) {
            if (count == 0)
                throw std::invalid_argument("Count must be positive");
            pos_x.resize(count);
            pos_y.resize(count);
            pos_z.resize(count);

            vx.resize(count);
            vy.resize(count);
            vz.resize(count);

            mass.resize(count);
            particles_id.resize(count);
        }

        std::size_t size() const noexcept {
            return pos_x.size();
        }

        void initializeParticles(){
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<double> pos(-100.0, 100.0);
            std::uniform_real_distribution<double> vel(-10.0, 10.0);
            std::uniform_real_distribution<double> mass_(0.1, 10);

            for(size_t i = 0; i < size();  i++){
                pos_x[i] = pos(gen);
                pos_y[i] = pos(gen);
                pos_z[i] = pos(gen);
                vx[i] = vel(gen);
                vy[i] = vel(gen);
                vz[i] = vel(gen);
                mass[i] = mass_(gen);
                particles_id[i] = static_cast<int>(i);
            }
        }

        double calculateTotalKineticEnergy() const {
            double totalEnergy = 0.0;
            for(size_t part_id = 0; part_id < size(); part_id++){
                double vel2 = vx[part_id]*vx[part_id] + vy[part_id]*vy[part_id] + vz[part_id]*vz[part_id];
                totalEnergy += 0.5 * mass[part_id] * vel2;
            }
            return totalEnergy;
        }
        
};



template <typename System>
double benchmark(System& system, int repetitions) {
    // Warm-up
    volatile double warmup =
        system.calculateTotalKineticEnergy();
    (void)warmup;

    double total_ms = 0.0;
    volatile double result = 0.0;

    for (int i = 0; i < repetitions; ++i) {
        const auto start = std::chrono::steady_clock::now();

        result = system.calculateTotalKineticEnergy();

        const auto end = std::chrono::steady_clock::now();

        total_ms += std::chrono::duration<double, std::milli>(
            end - start
        ).count();
    }

    // Consume the result so it remains observable.
    (void)result;

    return total_ms / repetitions;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0]
                  << " particle_count\n";
        return 1;
    }

    try {
        const long long parsed_count = std::stoll(argv[1]);

        if (parsed_count <= 0) {
            throw std::invalid_argument(
                "Particle count must be positive"
            );
        }

        const auto count = static_cast<std::size_t>(
            parsed_count
        );

        constexpr int repetitions = 100;

        ParticleSystemSoA soa;
        ParticleSystemAoS aos;

        soa.resize(count);
        aos.resize(count);

        soa.initializeParticles();
        aos.initializeParticles();

        const double energy_soa =
            soa.calculateTotalKineticEnergy();

        const double energy_aos =
            aos.calculateTotalKineticEnergy();

        std::cout << "Particles: " << count << '\n';
        std::cout << "SoA energy: " << energy_soa << '\n';
        std::cout << "AoS energy: " << energy_aos << '\n';

        const double time_soa = benchmark(soa, repetitions);
        const double time_aos = benchmark(aos, repetitions);

        std::cout << "\nBenchmark (" << repetitions
                  << " repetitions)\n";

        std::cout << "SoA average: " << time_soa << " ms\n";
        std::cout << "AoS average: " << time_aos << " ms\n";

        if (time_soa < time_aos) {
            std::cout << "SoA is faster by "
                      << time_aos / time_soa << "x\n";
        } else if (time_aos < time_soa) {
            std::cout << "AoS is faster by "
                      << time_soa / time_aos << "x\n";
        } else {
            std::cout << "Both have similar measured times.\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}