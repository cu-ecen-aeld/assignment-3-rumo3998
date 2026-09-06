#include "unity.h"
#include <stdbool.h>
#include <stdlib.h>
#include "../../examples/autotest-validate/autotest-validate.h"
#include "../../assignment-autotest/test/assignment1/username-from-conf-file.h"

/**
* This function should:
*   1) Call the my_username() function in Test_assignment_validate.c to get your hard coded username.
*   2) Obtain the value returned from function malloc_username_from_conf_file() in username-from-conf-file.h within
*       the assignment autotest submodule at assignment-autotest/test/assignment1/
*   3) Use unity assertion TEST_ASSERT_EQUAL_STRING_MESSAGE the two strings are equal.  See
*       the [unity assertion reference](https://github.com/ThrowTheSwitch/Unity/blob/master/docs/UnityAssertionsReference.md)
*/
void test_validate_my_username()
{
    /**
     * TODO: Replace the line below with your code here as described above to verify your /conf/username.txt 
     * config file and my_username() functions are setup properly
     */
    /* comment out previous assertion 
     * TEST_ASSERT_TRUE_MESSAGE(false,"AESD students, please fix me!");
     */
	//char ptr to user_name given by my_username()
	const char *charptr; 
	char *charptr2;

	//set the charptrs equal to the vals provided by my_usrname and malloc_usr..	
	charptr = my_username();
	charptr2 = malloc_username_from_conf_file();

	//use assertion to  determine string equality
	TEST_ASSERT_EQUAL_STRING_MESSAGE(charptr, charptr2, "FAILURE: usr name mismatch");
	//username-from-conf-file spec, charptr must be freed by user
	free(charptr2);
}
