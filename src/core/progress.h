#pragma once

#ifndef EVP_CORE_PROGRESS_H
#define EVP_CORE_PROGRESS_H

namespace evp {
	namespace core {
		constexpr int PROGRESS_UPDATE_PERCENT = 2;

		static float curr_prog = 0;
		static int prev_prog = 0;

		static void update_progress(float value, bool is_delta = true) {
			if(is_delta)
				curr_prog += value;
			else
				curr_prog = value;

			int progress = curr_prog / PROGRESS_UPDATE_PERCENT;
			if(progress <= prev_prog) return;

			prev_prog = progress;

			std::cout << "\r[";
			for(int i = 1; i <= 100 / PROGRESS_UPDATE_PERCENT; i++) {
				if(progress >= i)
					std::cout << "X";
				else
					std::cout << " ";
			}

			if(progress != 100 / PROGRESS_UPDATE_PERCENT && progress % 4 == 0)
				std::cout << "] Working    " << std::flush;
			else if(progress != 100 / PROGRESS_UPDATE_PERCENT && progress % 4 == 1)
				std::cout << "] Working.   " << std::flush;
			else if(progress != 100 / PROGRESS_UPDATE_PERCENT && progress % 4 == 2)
				std::cout << "] Working..  " << std::flush;
			else if(progress != 100 / PROGRESS_UPDATE_PERCENT && progress % 4 == 3)
				std::cout << "] Working... " << std::flush;
			else
				std::cout << "] Done       " << std::flush;
		}

		static void draw_progress_bar() {
			curr_prog = 0;
			prev_prog = 0;

			std::cout << "\r[";
			for(int i = 1; i <= 100 / PROGRESS_UPDATE_PERCENT; i++) {
				std::cout << " ";
			}
			std::cout << "] Working    " << std::flush;
		}
	}
}

#endif
