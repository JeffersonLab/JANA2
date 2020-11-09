//
// Created by Nathan Brei on 6/18/20.
//

#ifndef JANA2_JSTATUSBITS_H
#define JANA2_JSTATUSBITS_H

#include <map>
#include <mutex>
#include <sstream>
#include <iomanip>

template <typename T>
class JStatusBits {

	uint64_t m_status = 0L;
	static std::mutex m_mutex;
	static std::map<uint32_t, std::string> m_status_bit_descriptions;

public:

	void SetStatus(uint64_t status) { m_status = status; }

	uint64_t GetStatus() const { return m_status; }

	bool GetStatusBit(T bit) const
	{
		/// Return the present value of the specified status bit.
		/// The value of "bit" should be from 0-63.

		return (m_status>>int(bit)) & 0x01;
	}

	bool SetStatusBit(T bit, bool val=true)
	{
		/// Set the value of the specified status bit. If the
		/// second argument is passed, the bit will be set to
		/// that value. Otherwise, the bit will be set to "true".
		/// The value of "bit" should be from 0-63.
		/// The value of the status bit prior to  this call is
		/// returned.

		bool old_val = (m_status>>int(bit)) & 0x01;

		uint64_t mask = ((uint64_t)0x01)<<int(bit);

		if(val){
			// Set bit
			m_status |= mask;
		}else{
			// Clear bit
			m_status &= ~mask;
		}

		return old_val;
	}


	bool ClearStatusBit(T bit)
	{
		/// Clear the specified status bit.
		/// The value of "bit" should be from 0-63.
		/// This is equivalent to calling SetStatusBit(bit, false).
		/// The value of the status bit prior to  this call is
		/// returned.

		bool old_val = (m_status>>int(bit)) & 0x01;

		uint64_t mask = ((uint64_t)0x01)<<int(bit);

		m_status &= ~mask;

		return old_val;
	}


	void ClearStatus()
	{
		/// Clear all bits in the status word. This
		/// is equivalent to calling SetStatus(0).

		m_status = 0L;
	}


	static void SetStatusBitDescription(T bit, std::string description)
	{
		/// Set the description of the specified bit.
		/// The value of "bit" should be from 0-63.

		std::lock_guard<std::mutex> lock(m_mutex);
		m_status_bit_descriptions[bit] = description;
	}


	static std::string GetStatusBitDescription(T bit)
	{
		/// Get the description of the specified status bit.
		/// The value of "bit" should be from 0-63.

		std::string description("no description available");

		std::lock_guard<std::mutex> lock(m_mutex);
		auto iter = m_status_bit_descriptions.find(bit);
		if(iter != m_status_bit_descriptions.end()) description = iter->second;

		return description;
	}


	static void GetStatusBitDescriptions(std::map<uint32_t, std::string> &status_bit_descriptions)
	{
		/// Get the full list of descriptions of status bits.
		/// Note that the meaning of the bits is implementation
		/// specific and so descriptions are optional. It may be
		/// that some or none of the bits used have an associated description.

		std::lock_guard<std::mutex> lock(m_mutex);
		status_bit_descriptions = m_status_bit_descriptions;
	}


	std::string ToString() const
	{
		/// Generate a formatted string suitable for printing to the screen, including the
		/// entire word in both hexadecimal and binary along with descriptions.

		// Lock mutex to prevent changes to status_bit_descriptions while we
		// read from it.
		std::lock_guard<std::mutex> lock(m_mutex);

		std::stringstream ss;

		// Add status in hex first
		ss << "status: 0x" << std::hex << std::setw(sizeof(uint64_t)*2) << std::setfill('0') << m_status << std::dec <<std::endl;

		// Binary
		ss << std::setw(0) << "   bin |";
		for(int i=sizeof(uint64_t)*8-1; i>=0; i--){
			ss << ((m_status>>i) & 0x1);
			if((i%8)==0) ss << "|";
		}
		ss << std::endl;

		// 1-byte hex under binary
		ss << "   hex ";
		for(int i=sizeof(uint64_t) - 1; i>=0; i--){
			ss << std::hex << "   0x"<< std::setw(2) << ((m_status>>(i*8)) & 0xFF) << "  ";
		}
		ss << std::endl;

		// Descriptions for each bit that has a description or is set
		for(unsigned int i=0; i<sizeof(uint64_t)*8; i++){
			uint64_t val = ((m_status>>i) & 0x1);

			auto iter = m_status_bit_descriptions.find(i);

			if(iter != m_status_bit_descriptions.end() || val != 0){
				ss << std::dec << std::setw(2) << std::setfill(' ');
				ss << " " << val << " - [" << std::setw(2) << std::setfill(' ') << i << "] " << m_status_bit_descriptions[i] << std::endl;
			}
		}
		ss << std::endl;
		return ss.str();
	}

};

template <typename T>
std::mutex JStatusBits<T>::m_mutex;

template <typename T>
std::map<uint32_t, std::string> JStatusBits<T>::m_status_bit_descriptions;


#endif //JANA2_JSTATUSBITS_H
