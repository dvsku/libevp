#pragma once

namespace libevp::v1 {
	class format_desc {
	public:
		static inline char HEADER[60] = {
			53,50,53,99,49,55,97,54,97,55,99,102,98,99,100,55,53,52,49,50,101,99,100,48,54,57,100,52,
			98,55,50,99,51,56,57,0,16,0,0,0,78,79,82,77,65,76,95,80,65,67,75,95,84,89,80,69,100,0,0,0
		};

		static inline char RESERVED[16] = {
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
		};
	};

	static_assert(sizeof(format_desc) != 76, "libevp::v1::format_desc wrong size");

	static inline unsigned int HEADER_END_OFFSET		= 0x3C;
	static inline unsigned int DATA_START_OFFSET		= 0x4C;
	static inline unsigned int GAP_BETWEEN_FILE_DESC	= 0x24;
}