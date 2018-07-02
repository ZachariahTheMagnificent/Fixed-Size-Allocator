#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include "statistics.hpp"
//#define MULTITHREADING
#define USE_CUSTOM_ALLOCATOR
//#define RESERVE_MEMORY_AHEAD_OF_TIME

using zachariahs_world::math::statistics_type;
using zachariahs_world::math::get_statistics;

#if defined USE_CUSTOM_ALLOCATOR
#include "fixed-size-allocator.hpp"
using zachariahs_world::custom_allocators::fixed_size_allocator_type;
#endif

template<typename type>
using array_type =
#if defined USE_CUSTOM_ALLOCATOR
std::vector<type, fixed_size_allocator_type<type>>;
#else
std::vector<type>;
#endif

template<typename type>
array_type<type> operator+ ( const array_type<type>& lower, const array_type<type>& upper )
{
	array_type<type> combined;
#if defined RESERVE_MEMORY_AHEAD_OF_TIME
	combined.reserve ( lower.size ( ) + upper.size ( ) );
#endif

	for ( const auto& element : lower )
	{
		combined.push_back ( element );
	}
	for ( const auto& element : upper )
	{
		combined.push_back ( element );
	}

	return combined;
}

template<typename type>
array_type<type> operator+ ( const array_type<type>& array, const type& back )
{
	array_type<type> combined;
#if defined RESERVE_MEMORY_AHEAD_OF_TIME
	combined.reserve ( array.size ( ) + 1 );
#endif

	for ( const auto& element : array )
	{
		combined.push_back ( element );
	}

	combined.push_back ( back );

	return combined;
}
template<typename type>
array_type<type> operator+ ( const type& front, const array_type<type>& array )
{
	array_type<type> combined;
#if defined RESERVE_MEMORY_AHEAD_OF_TIME
	combined.reserve ( array.size ( ) + 1 );
#endif

	combined.push_back ( front );

	for ( const auto& element : array )
	{
		combined.push_back ( element );
	}
}

template<typename type>
array_type<type> quick_sort ( const array_type<type>& array )
{
	if ( array.size ( ) < 2 )
	{
		return array;
	}

	array_type<type> lower;
	array_type<type> upper;
	const type middle = array.front ( );

#if defined RESERVE_MEMORY_AHEAD_OF_TIME
	lower.reserve ( array.size ( ) );
	upper.reserve ( array.size ( ) );
#endif

	for ( auto i = std::size_t { } +1, size = array.size ( ); i < size; ++i )
	{
		if ( array [ i ] < middle )
		{
			lower.push_back ( array [ i ] );
		}
		else
		{
			upper.push_back ( array [ i ] );
		}
	}

	return quick_sort ( lower ) + middle + quick_sort ( upper );
}

int main ( )
{
	constexpr auto num_tests = 10;
	constexpr auto num_ints = 10000;
	constexpr auto num_frames = 100;
	constexpr auto lowest_int = int { 12 };
	constexpr auto highest_int = int { 758 };

	using clock = std::chrono::steady_clock;

	auto data_points = std::vector<unsigned long long> { };

	std::cout << "Circular Buffer Allocator benchmark test\n";
	std::system ( "pause" );
	std::cout << '\n';

	auto test = [ lowest_int, highest_int, num_ints, num_frames ]
	{
		auto rng_machine = std::mt19937 { };
		auto int_generator = std::uniform_int_distribution<int> { lowest_int, highest_int };
		auto ints = array_type<int> { };

		for ( auto i = std::size_t { }; i < num_frames; ++i )
		{
			ints = [ &rng_machine, &int_generator, &ints, num_ints ]
			{
				auto random_ints = array_type<int> { };
#if defined RESERVE_MEMORY_AHEAD_OF_TIME
				random_ints.reserve ( num_ints );
#endif

				for ( auto i = std::size_t { }; i < num_ints; ++i )
				{
					random_ints.push_back ( int_generator ( rng_machine ) );
				}

				return quick_sort ( random_ints );
			} ( );
		}
	};

	for ( auto i = std::size_t { }; i < num_tests; ++i )
	{
		// Warmup
		test ( );

		// Beginning of profile

		auto start = clock::now ( );
		test ( );
		auto end = clock::now ( );
		auto duration = end - start;
		auto duration_in_nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds> ( duration );
		auto count = duration_in_nanoseconds.count ( );
		data_points.push_back ( count );

		std::cout << "Test " << i + 1 << '/' << num_tests << " done!\n";
	}

	const auto statistics = get_statistics ( data_points );
	std::cout << "Programmed with: "
#if defined _WIN64
		<< "[_WIN64]"
#endif
#if defined MULTITHREADING
		<< "[MULTITHREADING]"
#endif
#if defined USE_CUSTOM_ALLOCATOR
		<< "[USE_CUSTOM_ALLOCATOR]"
#endif
#if defined RESERVE_MEMORY_AHEAD_OF_TIME
		<< "[RESERVE_MEMORY_AHEAD_OF_TIME]"
#endif
		<< '\n';
	std::cout << "Average: " << statistics.mean << "ns\n";
	std::cout << "Standard deviation: " << statistics.standard_deviation << "ns\n";
	std::cout << "Highest: " << statistics.highest << "ns\n";
	std::cout << "Lowest: " << statistics.lowest << "ns\n";
	std::cout << "Median: " << statistics.median << "ns\n";
	std::system ( "pause" );
	return 0;
}
