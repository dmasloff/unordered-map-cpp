build: test_simple test_simple_opt test_ubsan

test_simple: tests/unordered_map_test.cpp src/unordered_map.h
	clang++ -std=c++20 -gdwarf-4 -O0 -Wall -Wextra -Werror -o ./test_simple tests/unordered_map_test.cpp

test_simple_opt: tests/unordered_map_test.cpp src/unordered_map.h
	clang++ -std=c++20 -O2 -Wall -Wextra -Werror -o ./test_simple_opt tests/unordered_map_test.cpp

test_ubsan: tests/unordered_map_test.cpp src/unordered_map.h
	clang++ -std=c++20 -g -O0 -Wall -Wextra -Werror -fsanitize=undefined -o ./test_ubsan tests/unordered_map_test.cpp

info:
	clang++ --version
	clang-tidy --version
	clang-format --version
	valgrind --version

run: build
	@echo 'Run tests (simple)'
	time ./test_simple
	@echo 'Run tests (simple_opt)'
	time ./test_simple_opt
	@echo 'Run tests (ubsan)'
	time ./test_ubsan
	@echo 'Run tests (valgrind)'
	time valgrind --leak-check=yes --error-exitcode=1 ./test_simple 

lint:
	@echo 'Check code is formatted'
	bash -c "diff -u <(cat src/*.h) <(clang-format --style=file --Werror src/*.h)"
	@echo 'Run linter'
	clang-tidy --config "$(shell cat .clang-tidy)" --warnings-as-errors="*"  tests/unordered_map_test.cpp '-header-filter=.*' -- -std=c++20 -g -O0 -Wall -Wextra -Werror
	# @echo 'Check NOLINT is not used'
	# ! grep NOLINT unordered_map.h
	@echo 'Check std::unordered_map is not used'
	! grep std::unordered_map src/unordered_map.h
	@echo 'Check all TODOs are removed'
	! grep TODO src/unordered_map.h

test: info run lint
	@echo 'Great job!'

format:
	@echo 'Apply linter fixes'
	clang-tidy --config "$(shell cat .clang-tidy)" --fix tests/unordered_map_test.cpp '-header-filter=.*' -- -std=c++20 -g -O0 -Wall -Wextra -Werror
	@echo 'Apply formatter'
	clang-format --style=file -i *.h *.cpp

clean:
	rm test_simple test_simple_opt test_ubsan
