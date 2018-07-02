#pragma once
#include <cstdlib>
#include <memory>
#include <array>
#include <atomic>
#include <cassert>

namespace zachariahs_world
{
	namespace custom_allocators
	{
		namespace fixed_size_allocator_traits
		{
			constexpr auto max_alignment = std::size_t { 1024 };
			constexpr auto max_block_size = std::size_t { 1024 * 1024 };
			constexpr auto num_blocks_per_block_size = std::size_t { 1024 };
			constexpr std::size_t calculate_buffer_size ( const std::size_t block_size ) noexcept
			{
				const auto total_size = block_size * num_blocks_per_block_size;
				if ( block_size >= max_block_size )
				{
					return total_size;
				}
				else
				{
					return total_size + calculate_buffer_size ( block_size * 2 );
				}
			}
			constexpr std::size_t calculate_buffer_size ( ) noexcept
			{
				return calculate_buffer_size ( 1 );
			}
			constexpr std::size_t buffer_size = calculate_buffer_size ( );
			constexpr std::size_t calculate_num_sizes ( const std::size_t block_size ) noexcept
			{
				if ( block_size >= max_block_size )
				{
					return 1;
				}
				else
				{
					return 1 + calculate_num_sizes ( block_size * 2 );
				}
			}
			constexpr std::size_t calculate_num_sizes ( ) noexcept
			{
				return calculate_num_sizes ( 1 );
			}
			constexpr auto num_sizes = calculate_num_sizes ( );

			struct block_size_properties_type
			{
				block_size_properties_type ( ) = default;
				constexpr explicit block_size_properties_type ( const std::size_t block_size, const std::size_t taken_displacement, const std::size_t buffer_displacement ) noexcept : block_size { block_size },
					taken_displacement { taken_displacement },
					buffer_displacement { buffer_displacement }
				{

				}

				std::size_t block_size { };
				std::size_t taken_displacement { };
				std::size_t buffer_displacement { };
			};
			template<std::size_t block_size = 1>
			constexpr block_size_properties_type get_block_size_properties ( const std::size_t minimum_block_size ) noexcept
			{
				if constexpr ( block_size >= max_block_size )
				{
					return block_size_properties_type { block_size, 0, 0 };
				}
				else
				{
					if ( block_size < minimum_block_size )
					{
						const auto block_size_properties = get_block_size_properties<block_size * 2> ( minimum_block_size );
						return block_size_properties_type { block_size_properties.block_size, block_size_properties.taken_displacement + num_blocks_per_block_size, block_size_properties.buffer_displacement + block_size * num_blocks_per_block_size };
					}
					else
					{
						return block_size_properties_type { block_size, 0, 0 };
					}
				}
			}

			class globals_type
			{
			public:
				globals_type ( ) = default;

				char* allocate ( const std::size_t block_size )
				{
					assert ( block_size <= max_block_size );

					const auto block_size_properties = get_block_size_properties ( block_size );

					for ( std::size_t index = block_size_properties.taken_displacement, stop_index = block_size_properties.taken_displacement + num_blocks_per_block_size; index != stop_index; ++index )
					{
						if ( !taken_ [ index ].test_and_set ( std::memory_order_acq_rel ) )
						{
							return &buffer_ [ 0 ] + block_size_properties.buffer_displacement + ( index - block_size_properties.taken_displacement ) * block_size_properties.block_size;
						}
					}

					throw std::bad_alloc { };
				}
				void deallocate ( char*const block_pointer, const std::size_t block_size ) noexcept
				{
					assert ( block_pointer >= &buffer_ [ 0 ] && block_pointer < &buffer_ [ 0 ] + buffer_size );
					assert ( block_size <= max_block_size );

					const auto block_size_properties = get_block_size_properties ( block_size );
					const auto buffer_displacement = reinterpret_cast<std::size_t> ( block_pointer ) - reinterpret_cast<std::size_t> ( &buffer_ [ 0 ] );
					const auto subbuffer_displacement = buffer_displacement - block_size_properties.buffer_displacement;
					const auto block_index = subbuffer_displacement / block_size_properties.block_size;
					const auto taken_flag_displacement = block_size_properties.taken_displacement + block_index;

					assert ( taken_ [ taken_flag_displacement ].test_and_set ( ) );

					taken_ [ taken_flag_displacement ].clear ( std::memory_order_release );
				}

			private:
				class buffer_deleter
				{
				public:
					void operator() ( char*const buffer ) const noexcept
					{
						_aligned_free ( buffer );
					}
				};
				std::unique_ptr<char [ ], buffer_deleter> buffer_ { static_cast<char*> ( _aligned_malloc ( buffer_size, max_alignment ) ) };
				std::unique_ptr<std::atomic_flag [ ]> taken_ = std::make_unique<std::atomic_flag [ ]> ( num_blocks_per_block_size * num_sizes );
			};
			inline auto globals = std::make_unique<globals_type> ( );
		}

		template<typename type>
		class fixed_size_allocator_type
		{
		public:
			static_assert ( alignof ( type ) <= fixed_size_allocator_traits::max_alignment );
			using value_type = type;

			fixed_size_allocator_type ( ) = default;

			template<typename other_type>
			constexpr fixed_size_allocator_type ( const fixed_size_allocator_type<other_type>& ) noexcept
			{

			}
			template<typename other_type>
			constexpr fixed_size_allocator_type ( fixed_size_allocator_type<other_type>&& ) noexcept
			{

			}

			template<typename other_type>
			constexpr fixed_size_allocator_type& operator= ( const fixed_size_allocator_type<other_type>& ) noexcept
			{

			}
			template<typename other_type>
			constexpr fixed_size_allocator_type& operator= ( fixed_size_allocator_type<other_type>&& ) noexcept
			{

			}

			template<typename other_type>
			constexpr bool operator== ( const fixed_size_allocator_type<other_type>& ) noexcept
			{
				return true;
			}
			template<typename other_type>
			constexpr bool operator!= ( const fixed_size_allocator_type<other_type>& ) noexcept
			{
				return false;
			}

			static constexpr std::size_t max_size ( ) noexcept
			{
				return fixed_size_allocator_traits::max_block_size;
			}

			value_type* allocate ( const std::size_t size )
			{
				return reinterpret_cast<value_type*> ( fixed_size_allocator_traits::globals->allocate ( size * sizeof ( value_type ) ) );
			}
			void deallocate ( value_type*const memory, const std::size_t size ) noexcept
			{
				fixed_size_allocator_traits::globals->deallocate ( reinterpret_cast<char*> ( memory ), size * sizeof ( value_type ) );
			}
		};
	}
}
