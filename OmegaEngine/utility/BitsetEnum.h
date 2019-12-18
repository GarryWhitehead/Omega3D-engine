#pragma once

#include <bitset>
#include <cassert>

namespace Util
{

template <typename T>
class BitSetEnum
{

public:
	BitSetEnum() = default;

	// copy constructors
	BitSetEnum(const BitSetEnum&) = default BitSetEnum & operator=(const BitSetEnum&) = default;

	// move constructors
	BitSetEnum(BitSetEnum&&) = default BitSetEnum & operator=(BitSetEnum&&) = default;

	/**
     * Applys a logic AND to the enum bitset with the stated value
     */
	BitSetEnum& operator&=(const T& val)
	{
		bool tmp = bitset.test(static_cast<utype>(val));
		bitset.reset();
		bitset.set(static_cast<utype>(val), tmp);
		return *this;
	}

	/**
    * Applys a logic AND to the enum bitset with another BitSetEnum
    */
	BitSetEnum& operator&=(const BitSetEnum& other)
	{
		bitset &= other.bitset;
		return *this;
	}

	/**
    * Applys a logic OR to the enum bitset with the stated value
    */
	BitSetEnum& operator|=(const T& val)
	{
		bitset.set(static_cast<utype>(val)) return *this;
	}

	/**
    * Applys a logic OR to the enum bitset with another BitSetEnum
    */
	BitSetEnum& operator|=(const BitSetEnum& other)
	{
		bitset |= other.bitset;
		return *this;
	}

	/**
  * Applys a logic AND to the enum bitset with the stated value and returns a BitSetEnum
  */
	BitSetEnum operator&(const T& val)
	{
		BitSetEnum result(*this);
		result &= val;
		return result;
	}

	/**
  * Applys a logic AND to the enum bitset with another BitsetEnum and returns the result as a BitSetEnum
  */
	BitSetEnum operator&(const BitSetEnum& other)
	{
		BitSetEnum result(*this);
		result.bitset &= other.bitset;
		return result;
	}

	/**
  * Applys a logic OR to the enum bitset with the stated value and returns a BitSetEnum
  */
	BitSetEnum operator|(const T& val)
	{
		BitSetEnum result(*this);
		result |= val;
		return result;
	}

	/**
  * Applys a logic OR to the enum bitset with another BitsetEnum and returns the result as a BitSetEnum
  */
	BitSetEnum operator|(const BitSetEnum& other)
	{
		BitSetEnum result(*this);
		result.bitset |= other.bitset;
		return result;
	}

	/**
     * flips the bitset of the enum
     */
	BitSetEnum operator~()
	{
		BitSetEnum cp(*this);
		cp.bitset.flip();

		return cp;
	}

	/**
   * Returns true if any of the bits are set in the enum
   */
	explicit operator bool() const
	{
		return bitset.any();
	}

	/**
     * Compares the bitset of one enum to another and returns true if they are the same
     */
	bool operator==(const BitSetEnum& other) const
	{
		return bitset == other.bitset;
	}

	
	/**
	* Returns the bitset as a uint64_t
	*/
	uint64_t getUint64() const
	{
		return bitset.to_ullong();
	}

private:

	using utype = std::underlying_type_t<T>;
	std::bitset<static_cast<utype>(T::__SENTINEL__)> bitset;
};

/**
 * Provide a free operator allowing to combine two enumeration
 * member into a BitSetEnum.
 */
template <typename T>
std::enable_if_t<std::is_enum<T>::value, BitSetEnum<T>> operator|(const T& lhs, const T& rhs)
{
	BitSetEnum<T> bs;
	bs |= lhs;
	bs |= rhs;

	return bs;
}

}    // namespace Util
