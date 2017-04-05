#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcre.h>

int main(int argc, char **argv) {
	pcre       *re_compiled;
	pcre_extra *p_extra;
	int         pcre_exec_ret;
	int         sub_str_vec[30];
	const char *pcre_error_str;
	int         pcre_error_offset;
	char       *str_regex;
	char      **to_match;
	const char *psub_str_match_str;
	int         j;
	
	char *test_strings[] = { "This should match... hello",
	                         "This could match... hello!",
	                         "More than one hello.. hello",
	                         "No chance of a match...",
	                         "This has more matches hellohello",
	                         NULL};


	str_regex = "(.*)(hello)+";

	// "(sens|respons)(e|ibility)"
	printf("Regex to use: %s\n", str_regex);

	// First, the regex string must be compiled.
	re_compiled = pcre_compile(str_regex, PCRE_UNGREEDY, &pcre_error_str, &pcre_error_offset, NULL);

	// OPTIONS (second argument) (|'ed together) can be:
	//    PCRE_ANCHORED       -- Like adding ^ at start of pattern.
	//    PCRE_CASELESS       -- Like m//i
	//    PCRE_DOLLAR_ENDONLY -- Make $ match end of string regardless of \n's
	//    No Perl equivalent.
	//    PCRE_DOTALL         -- Makes . match newlines too.  Like m//s
	//    PCRE_EXTENDED       -- Like m//x
	//    PCRE_EXTRA          -- 
	//    PCRE_MULTILINE      -- Like m//m
	//    PCRE_UNGREEDY       -- Set quantifiers to be ungreedy.  Individual quantifiers
	//                           may be set to be greedy if they are followed by "?".
	//    PCRE_UTF8           -- Work with UTF8 strings.

	// pcre_compile returns NULL on error, and sets pcre_error_offset & pcre_error_str
	if(re_compiled == NULL) {
		printf("ERROR: Could not compile '%s': %s\n", str_regex, pcre_error_str);
		exit(1);
	}

	// Optimize the regex
	p_extra = pcre_study(re_compiled, 0, &pcre_error_str);

	// pcre_study() returns NULL for both errors and when it can not optimize
	//    the regex.  The last argument is how one checks for errors (it is NULL
	//    if everything works, and points to an error string otherwise.
	if(pcre_error_str != NULL) {
		printf("ERROR: Could not study '%s': %s\n", str_regex, pcre_error_str);
		exit(1);
	}

	for(to_match = test_strings; *to_match != NULL; to_match++) {
		printf("String: %s\n", *to_match);
		printf("        %s\n", "0123456789012345678901234567890123456789");
		printf("        %s\n", "0         1         2         3");

		// Try to find the regex in to_match, and report results.
		pcre_exec_ret = pcre_exec(re_compiled,
		                          p_extra,
		                          *to_match, 
		                          strlen(*to_match),  // length of string
		                          0,                  // Start looking at this point
		                          0,                  // OPTIONS
		                          sub_str_vec,
		                          30);                // Length of sub_str_vec

		// pcre_exec OPTIONS (|'ed together) can be:
		//    PCRE_ANCHORED -- can be turned on at this time.
		//    PCRE_NOTBOL
		//    PCRE_NOTEOL
		//    PCRE_NOTEMPTY

		// Report what happened in the pcre_exec call..
		//printf("pcre_exec return: %d\n", pcre_exec_ret);
		if(pcre_exec_ret < 0) { // Something bad happened..
			switch(pcre_exec_ret) {
			case PCRE_ERROR_NOMATCH: 
				printf("String did not match the pattern\n");
				break;
			case PCRE_ERROR_NULL: 
				printf("Something was null\n");
				break;
			case PCRE_ERROR_BADOPTION:
				printf("A bad option was passed\n");
				break;
			case PCRE_ERROR_BADMAGIC: 
				printf("Magic number bad (compiled re corrupt?)\n");
				break;
			case PCRE_ERROR_UNKNOWN_NODE:
				printf("Something kooky in the compiled re\n");
				break;
			case PCRE_ERROR_NOMEMORY: 
				printf("Ran out of memory\n");
				break;
			default: 
				printf("Unknown error\n");
				break;
			} /* end switch */
		} else {
			printf("Result: We have a match!\n");
        
			// At this point, rc contains the number of substring matches found...
			if(pcre_exec_ret == 0) {
				printf("But too many substrings were found to fit in sub_str_vec!\n");
				// Set rc to the max number of substring matches possible.
				pcre_exec_ret = 30 / 3;
			}

			// DIY way to get the first substring match (whole pattern):
			// char subStrMatchStr[1024];
			// int i, j
			// for(j=0,i=sub_str_vec[0];i<sub_str_vec[1];i++,j++) 
			//   subStrMatchStr[j] = (*to_match)[i];
			// subStrMatchStr[sub_str_vec[1]-sub_str_vec[0]] = 0;
			//printf("MATCHED SUBSTRING: '%s'\n", subStrMatchStr);
        
			// PCRE contains a handy function to do the above for you:
			for(j = 0; j < pcre_exec_ret; j++) {
				pcre_get_substring(*to_match, sub_str_vec, 
				                   pcre_exec_ret, j, &(psub_str_match_str));
				printf("Match(%2d/%2d): (%2d,%2d): '%s'\n", j, pcre_exec_ret - 1, 
				       sub_str_vec[j * 2], sub_str_vec[j * 2 + 1], psub_str_match_str);
			
			}
			// Free up the substring
			pcre_free_substring(psub_str_match_str);
		}
		printf("\n");
      
	} 
  
	// Free up the regular expression.
	pcre_free(re_compiled);
      
	// Free up the EXTRA PCRE value (may be NULL at this point)
	if(p_extra != NULL)
		pcre_free(p_extra);

	// We are all done..
	return 0;
}
