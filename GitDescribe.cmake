
function(git_description VAR)
	find_package(Git)

	execute_process(
		COMMAND "${GIT_EXECUTABLE}" describe --long
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		RESULT_VARIABLE _result
		OUTPUT_VARIABLE _full
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	if (_result EQUAL 0)
		string(REPLACE "-" ";" _list "${_full}")
		list(REVERSE _list)
		list(GET _list 0 _sha)
		list(REMOVE_AT _list 0 1)
		list(REVERSE _list)
		string(REPLACE ";" "-" _tagname "${_list}")
		set(_output "git-${_tagname}-${_sha}")
	else()
		execute_process(
			COMMAND "${GIT_EXECUTABLE}" describe --always
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			RESULT_VARIABLE _result
			OUTPUT_VARIABLE _sha
			ERROR_QUIET
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)

		if (_result EQUAL 0)
			set(_output "git-${_sha}")
		endif()
	endif()

	if (_output)
		execute_process(
			COMMAND "${GIT_EXECUTABLE}" diff-index --quiet HEAD --
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			RESULT_VARIABLE _result
			OUTPUT_QUIET
			ERROR_QUIET
		)
		if(NOT _result EQUAL 0)
			set(_output "${_output}-dirty")
		endif()

		set(${VAR} ${_output} PARENT_SCOPE)
	endif()

endfunction(git_description)