#include <concepts>
#include <ranges>
#include <vector>
#include <algorithm>
#include <numeric>
#include <functional>
#include <tuple>
#include <type_traits>
#include <cstddef>
#include<iostream>
#include <string>
#include <string_view>
#include <cctype>


// custom type
struct Point3D{
    double x;
    double y;
    double z;
};

struct Particle{
    int id;
    double mass;
    Point3D position;
    Point3D velocity;
};

// Mesh cell
struct MeshCell
{
    int id;
    double volume;
    double density;
};

// Define concepts for type constraints
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template<typename T>
concept Comparable = requires(T a, T b) {
    { a < b } -> std::convertible_to<bool>;
    { a > b } -> std::convertible_to<bool>;
};

template<typename Container>
concept NumericContainer =
    std::ranges::input_range<const Container> &&
    Numeric<std::ranges::range_value_t<Container>>;


template<typename T>
concept StringLike = std::ranges::contiguous_range<T> && std::ranges::sized_range<T> &&
    std::same_as<std::remove_cv_t<std::ranges::range_value_t<T>>, char>;

template<typename Container>
concept StringContainer =
    std::ranges::input_range<const Container> &&
    StringLike<std::ranges::range_value_t<Container>>;


template<typename T>
concept NonEmptyStringLike = StringLike<T> && requires(const T& str){
    {str.empty()} -> std::convertible_to<bool>;
};


template<typename T>
concept Point3DLike = requires(const T& p){
    requires Numeric<decltype(p.x)>;
    requires Numeric<decltype(p.y)>;
    requires Numeric<decltype(p.z)>;
};

template<typename T>
concept ParticleLike = requires(const T& p){
    requires std::integral<decltype(p.id)>;
    requires Numeric<decltype(p.mass)>;
    requires Point3DLike<std::remove_cvref_t<decltype(p.position)>>;
    requires Point3DLike<std::remove_cvref_t<decltype(p.velocity)>>;
};

template<typename T>
concept MeshCellLike = requires(const T& cell)
{
    requires std::integral<decltype(cell.id)>;
    requires Numeric<decltype(cell.volume)>;
    requires Numeric<decltype(cell.density)>;
};

template<typename T>
concept GenericGeometry = requires (const T& p){
    requires Numeric<std::remove_cvref_t<decltype(p.volume)>>;
    requires Point3DLike<std::remove_cvref_t<decltype(p.center)>>;
};


template<typename Range>
concept ParticleRange =
    std::ranges::input_range<const Range> &&
    ParticleLike<std::ranges::range_value_t<Range>>;

template<typename Range>
concept MeshCellRange =
    std::ranges::input_range<const Range> &&
    MeshCellLike<std::ranges::range_value_t<Range>>;

class DataProcessor {
public:
    // Process numeric data with concepts-constrained templates
    template<NumericContainer Container>
    auto analyzeData(const Container& data) {
         using ValueType = std::ranges::range_value_t<Container>;
    
        
        // Use ranges for functional-style processing
        auto positiveValues = data 
            | std::views::filter([](const auto& x) { return x > 0; });
        
        auto squares = positiveValues 
            | std::views::transform([](const auto& x) { return x * x; });
        
        // Calculate statistics
        ValueType sum = std::accumulate(squares.begin(), squares.end(), ValueType{0});
        size_t count = std::distance(squares.begin(), squares.end());
        
        return std::make_tuple(sum, count, count > 0 ? sum / count : 0);
    }

    // Concepts-constrained sorting function
    // Sorting
    template<Comparable T>
    requires std::sortable<typename std::vector<T>::iterator>
    void advancedSort(std::vector<T>& data, bool ascending = true)
    {
        if (ascending)
        {
            std::ranges::sort(data);
        }
        else
        {
            std::ranges::sort(
                data,
                std::greater<>{}
            );
        }
    }

    // Generic filter with concepts
    template<NumericContainer Container, typename Predicate>
    requires std::predicate<Predicate&, std::ranges::range_reference_t<const Container>>
    auto filterData(const Container& data, Predicate pred) 
    {
       using ValueType =
            std::ranges::range_value_t<Container>;

        std::vector<ValueType> result;

        for (const auto& value : data)
        {
            if (std::invoke(pred, value))
            {
                result.push_back(value);
            }
        }

        return result;
    
    }

    // --------------------------------------------------
    // String processing
    // --------------------------------------------------

    template<StringLike String>
    auto countCharacter(const String& data, char character)
    {
        return std::ranges::count(data, character);
    }


    // Filter a container of strings
    template<StringContainer Container, typename Predicate>
    requires std::predicate<Predicate&, std::ranges::range_reference_t<const Container>>
    auto filterStrings(const Container& data,Predicate pred)
    {
        using ValueType = std::ranges::range_value_t<Container>;

        std::vector<ValueType> result;

        for (const auto& value : data)
        {
            if (std::invoke(pred, value))
            {
                result.push_back(value);
            }
        }

        return result;
    }


    // Transform strings
    template<StringContainer Container, typename Transform>
    requires std::invocable<Transform&, std::ranges::range_reference_t<const Container>>
    auto transformStrings(const Container& data,Transform transform)
    {
        using ResultType = std::invoke_result_t<Transform&, std::ranges::range_reference_t<const Container>>;

        std::vector<ResultType> result;

        for (const auto& value : data)
        {
            result.push_back(std::invoke(transform, value));
        }

        return result;
    }

    template<NonEmptyStringLike String>
    bool isNonEmpty(const String& data)
    {
        return !data.empty();
    }

    // processing Point3D
    template<Point3DLike Point>
    auto magnitude(const Point& p)
    {
        return std::hypot(
            static_cast<long double>(p.x),
            static_cast<long double>(p.y),
            static_cast<long double>(p.z)
        );
    }

    // processing particles
    template<ParticleLike P>
    auto kineticEnergy(const P& particle)
    {
        const auto speed = magnitude(particle.velocity);

        return 0.5L *
            static_cast<long double>(particle.mass) *
            speed * speed;
    }

    template<ParticleRange Range>
    auto totalKineticEnergy(const Range& particles)
    {
        long double total = 0.0L;

        for (const auto& particle : particles)
            total += kineticEnergy(particle);

        return total;
    }


    // processing MeshCell
    template<MeshCellLike Cell>
    auto cellMass(const Cell& cell)
    {
        return static_cast<long double>(cell.density) * static_cast<long double>(cell.volume);
    }
    
    template<MeshCellRange Range>
    auto totalMeshMass(const Range& cells)
    {
        long double total = 0.0L;

        for (const auto& cell : cells)
            total += cellMass(cell);

        return total;
    }


};

int main()
{
    DataProcessor processor;

    std::vector<double> data =
    {
        -2.0, 1.0, 2.0, 3.0, 4.0, -1.0
    };

    auto [sum, count, mean] =
        processor.analyzeData(data);

    std::cout << "Sum of squares: " << sum << '\n';
    std::cout << "Count: " << count << '\n';
    std::cout << "Mean: " << mean << '\n';

    // Single string
    std::string text = "Scientific Computing";

    auto count_occ = processor.countCharacter(text, 'i');

    std::cout << "Occurrences: " << count_occ << '\n';


    // Container of strings
    std::vector<std::string> words =
    {
        "Kokkos",
        "CUDA",
        "OpenMP",
        "C++20",
        "Arcane"
    };


    // Filter strings
    auto filtered = processor.filterStrings(
        words,
        [](const std::string& word)
        {
            return word.size() > 4;
        }
    );


    // Transform strings to uppercase
    auto uppercase = processor.transformStrings(
        filtered,
        [](const std::string& word)
        {
            std::string result = word;

            std::ranges::transform(
                result,
                result.begin(),
                [](unsigned char c)
                {
                    return static_cast<char>(
                        std::toupper(c)
                    );
                }
            );

            return result;
        }
    );


    for (const auto& word : uppercase)
    {
        std::cout << word << '\n';
    }

    Point3D p{3.0, 4.0, 2.0};

    std::cout << processor.magnitude(p) << '\n';

    Particle particle
    {
        1,
        2.0,
        {0.0, 0.0, 0.0},
        {3.0, 4.0, 2.0}
    };

    std::cout << processor.kineticEnergy(particle) << '\n';

    MeshCell cell
    {
        10,
        2.5,
        1000.0
    };

    std::cout << processor.cellMass(cell) << '\n';


    return 0;
}