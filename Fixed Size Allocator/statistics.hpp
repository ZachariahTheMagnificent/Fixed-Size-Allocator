#pragma once
#include <algorithm>
#include <numeric>

namespace zachariahs_world
{
	namespace math
	{
		template<typename num_type>
		struct statistics_type
		{
			statistics_type ( ) = default;
			constexpr explicit statistics_type ( const num_type lowest, const num_type highest, const num_type median, const num_type mean, const num_type standard_deviation ) noexcept : lowest { lowest },
				highest { highest },
				median { median },
				mean { mean },
				standard_deviation { standard_deviation }
			{

			}

			num_type lowest = num_type { };
			num_type highest = num_type { };
			num_type median = num_type { };
			num_type mean = num_type { };
			num_type standard_deviation = num_type { };
		};

		template<typename data_points_array>
		constexpr auto get_statistics ( data_points_array data_points )
		{
			std::sort ( std::begin ( data_points ), std::end ( data_points ) );

			const auto begin = std::cbegin ( data_points );
			const auto end = std::cend ( data_points );
			const auto size = std::size ( data_points );

			using num_type = std::remove_all_extents_t<decltype ( *begin )>;

			const auto lowest = *begin;
			const auto highest = *end;
			const auto median = data_points [ size / 2 ];
			const auto sum = std::accumulate ( begin, end, num_type { } );
			const auto mean = sum / size;
			const auto standard_deviation = static_cast<num_type> ( std::sqrt ( std::accumulate ( begin, end, num_type { }, [ mean ] ( const auto total, const auto point )
			{
				const auto difference = point - mean;
				return total + difference * difference;
			} ) / size ) );

			return statistics_type {
				lowest,
				highest,
				median,
				mean,
				standard_deviation };
		}
	}
}
