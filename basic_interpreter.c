#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 256
#define MAX_VARS 26
#define MAX_ARRAYS 26
#define MAX_ARRAY_SIZE 1000
#define MAX_FOR_STACK 10

/* Program storage */
typedef struct {
    int line_number;
    char text[MAX_LINE_LENGTH];
} ProgramLine;

ProgramLine program[MAX_LINES];
int program_size = 0;

/* Variables A-Z */
int variables[MAX_VARS];

/* String variables A-Z */
char *string_variables[MAX_VARS];

/* Arrays A-Z */
typedef struct {
    int *data;
    int size;
    bool allocated;
} Array;

Array arrays[MAX_ARRAYS];

/* FOR loop stack */
typedef struct {
    char var_name;
    int end_value;
    int step_value;
    int line_index;
} ForStackEntry;

ForStackEntry for_stack[MAX_FOR_STACK];
int for_stack_ptr = 0;

/* Parser state */
char *current_pos;
int current_line_index;

/* Function prototypes */
void init_interpreter(void);
void cleanup_interpreter(void);
void run_program(void);
void list_program(void);
void clear_program(void);
int parse_expression(void);
int parse_term(void);
int parse_factor(void);
void skip_whitespace(void);
void execute_line(int line_index);
void execute_print(void);
void execute_let(void);
void execute_goto(void);
void execute_if(void);
void execute_dim(void);
void execute_input(void);
void execute_for(void);
void execute_next(void);
void skip_to_next(char var_name);
int find_line(int line_number);
void insert_line(int line_number, const char *text);
void save_program(const char *filename);
void load_program(const char *filename);
char *read_string_literal(void);
char *parse_string_operand(void);
int get_array_index(char var_name);

/* Initialize interpreter */
void init_interpreter(void) {
    int i;
    
    /* Initialize variables to 0 */
    for (i = 0; i < MAX_VARS; i++) {
        variables[i] = 0;
    }
    
    /* Initialize string variables */
    for (i = 0; i < MAX_VARS; i++) {
        string_variables[i] = NULL;
    }

    /* Initialize arrays */
    for (i = 0; i < MAX_ARRAYS; i++) {
        arrays[i].data = NULL;
        arrays[i].size = 0;
        arrays[i].allocated = false;
    }
    
    /* Reset FOR stack */
    for_stack_ptr = 0;

    program_size = 0;
}

/* Cleanup interpreter */
void cleanup_interpreter(void) {
    int i;
    for (i = 0; i < MAX_ARRAYS; i++) {
        if (arrays[i].allocated && arrays[i].data != NULL) {
            free(arrays[i].data);
            arrays[i].data = NULL;
            arrays[i].allocated = false;
        }
    }
    for (i = 0; i < MAX_VARS; i++) {
        if (string_variables[i]) {
            free(string_variables[i]);
            string_variables[i] = NULL;
        }
    }
}

/* Skip whitespace */
void skip_whitespace(void) {
    while (*current_pos && isspace(*current_pos)) {
        current_pos++;
    }
}

/* Parse factor: number, variable, array element, or parenthesized expression */
int parse_factor(void) {
    skip_whitespace();
    
    if (*current_pos == '(') {
        current_pos++;
        int result = parse_expression();
        skip_whitespace();
        if (*current_pos == ')') {
            current_pos++;
        }
        return result;
    }
    
    if (isalpha(*current_pos)) {
        if (strncasecmp(current_pos, "INSTR", 5) == 0) {
            current_pos += 5;
            skip_whitespace();
            if (*current_pos == '(') {
                current_pos++;
                char *haystack = parse_string_operand();
                skip_whitespace();
                if (*current_pos == ',') {
                    current_pos++;
                    char *needle = parse_string_operand();
                    skip_whitespace();
                    if (*current_pos == ')') {
                        current_pos++;
                        int result = 0;
                        if (haystack && needle) {
                            char *found = strstr(haystack, needle);
                            if (found) {
                                result = (int)(found - haystack) + 1;
                            }
                        }
                        if (haystack) free(haystack);
                        if (needle) free(needle);
                        return result;
                    }
                    if (needle) free(needle);
                }
                if (haystack) free(haystack);
            }
            return 0;
        }

        char var_name = toupper(*current_pos);
        current_pos++;
        skip_whitespace();
        
        /* Check for array subscript */
        if (*current_pos == '[' || *current_pos == '(') {
            char closing = (*current_pos == '[') ? ']' : ')';
            current_pos++;
            int index = parse_expression();
            skip_whitespace();
            if (*current_pos == closing) {
                current_pos++;
            }
            
            int arr_idx = var_name - 'A';
            if (!arrays[arr_idx].allocated) {
                fprintf(stderr, "Error: Array %c not dimensioned\n", var_name);
                return 0;
            }
            if (index < 0 || index >= arrays[arr_idx].size) {
                fprintf(stderr, "Error: Array index %d out of bounds for %c\n", index, var_name);
                return 0;
            }
            return arrays[arr_idx].data[index];
        }
        
        return variables[var_name - 'A'];
    }
    
    if (isdigit(*current_pos) || (*current_pos == '-' && isdigit(*(current_pos + 1)))) {
        int sign = 1;
        if (*current_pos == '-') {
            sign = -1;
            current_pos++;
        }
        int result = 0;
        while (isdigit(*current_pos)) {
            result = result * 10 + (*current_pos - '0');
            current_pos++;
        }
        return sign * result;
    }
    
    return 0;
}

/* Parse term: factor with *, / */
int parse_term(void) {
    int result = parse_factor();
    
    while (1) {
        skip_whitespace();
        if (*current_pos == '*') {
            current_pos++;
            result *= parse_factor();
        } else if (*current_pos == '/') {
            current_pos++;
            int divisor = parse_factor();
            if (divisor != 0) {
                result /= divisor;
            } else {
                fprintf(stderr, "Error: Division by zero\n");
            }
        } else {
            break;
        }
    }
    
    return result;
}

/* Parse expression: term with +, - */
int parse_expression(void) {
    int result = parse_term();
    
    while (1) {
        skip_whitespace();
        if (*current_pos == '+') {
            current_pos++;
            result += parse_term();
        } else if (*current_pos == '-') {
            current_pos++;
            result -= parse_term();
        } else {
            break;
        }
    }
    
    return result;
}

/* Read string literal */
char *read_string_literal(void) {
    static char buffer[MAX_LINE_LENGTH];
    int i = 0;
    
    skip_whitespace();
    if (*current_pos != '"') {
        return NULL;
    }
    
    current_pos++; /* Skip opening quote */
    while (*current_pos && *current_pos != '"' && i < MAX_LINE_LENGTH - 1) {
        buffer[i++] = *current_pos++;
    }
    buffer[i] = '\0';
    
    if (*current_pos == '"') {
        current_pos++; /* Skip closing quote */
    }
    
    return buffer;
}

/* Execute PRINT statement */
void execute_print(void) {
    bool first = true;
    
    while (1) {
        skip_whitespace();
        
        if (!*current_pos || *current_pos == '\n') {
            break;
        }
        
        if (!first) {
            printf(" ");
        }
        first = false;
        
        char *save_pos = current_pos;
        char *str = parse_string_operand();
        if (str) {
            printf("%s", str);
            free(str);
        } else {
            current_pos = save_pos;
            printf("%d", parse_expression());
        }
        
        skip_whitespace();
        if (*current_pos == ',') {
            current_pos++;
        }
    }
    
    printf("\n");
}

/* Execute LET statement */
void execute_let(void) {
    skip_whitespace();
    
    if (!isalpha(*current_pos)) {
        fprintf(stderr, "Error: Expected variable name\n");
        return;
    }
    
    char var_name = toupper(*current_pos);
    current_pos++;
    skip_whitespace();

    if (*current_pos == '$') {
        current_pos++;
        skip_whitespace();
        if (*current_pos == '=') {
            current_pos++;
        }
        char *val = parse_string_operand();
        int idx = var_name - 'A';
        if (string_variables[idx]) {
            free(string_variables[idx]);
            string_variables[idx] = NULL;
        }
        if (val) {
            string_variables[idx] = val;
        } else {
             /* Assignment of empty or invalid string */
            string_variables[idx] = (char *)malloc(1);
            if (string_variables[idx]) *string_variables[idx] = '\0';
        }
        return;
    }
    
    /* Check for array assignment */
    if (*current_pos == '[' || *current_pos == '(') {
        char closing = (*current_pos == '[') ? ']' : ')';
        current_pos++;
        int index = parse_expression();
        skip_whitespace();
        if (*current_pos == closing) {
            current_pos++;
        }
        
        skip_whitespace();
        if (*current_pos == '=') {
            current_pos++;
        }
        
        int arr_idx = var_name - 'A';
        if (!arrays[arr_idx].allocated) {
            fprintf(stderr, "Error: Array %c not dimensioned\n", var_name);
            return;
        }
        if (index < 0 || index >= arrays[arr_idx].size) {
            fprintf(stderr, "Error: Array index %d out of bounds for %c\n", index, var_name);
            return;
        }
        
        arrays[arr_idx].data[index] = parse_expression();
        return;
    }
    
    if (*current_pos == '=') {
        current_pos++;
    }
    
    variables[var_name - 'A'] = parse_expression();
}

/* Execute DIM statement */
void execute_dim(void) {
    skip_whitespace();
    
    if (!isalpha(*current_pos)) {
        fprintf(stderr, "Error: Expected array name\n");
        return;
    }
    
    char var_name = toupper(*current_pos);
    current_pos++;
    skip_whitespace();
    
    char closing = 0;
    if (*current_pos == '[' || *current_pos == '(') {
        closing = (*current_pos == '[') ? ']' : ')';
        current_pos++;
    }
    
    int size = parse_expression();
    skip_whitespace();
    
    if (closing && *current_pos == closing) {
        current_pos++;
    }
    
    int arr_idx = var_name - 'A';
    
    if (arrays[arr_idx].allocated) {
        fprintf(stderr, "Error: Array %c already dimensioned\n", var_name);
        return;
    }
    
    if (size <= 0 || size > MAX_ARRAY_SIZE) {
        fprintf(stderr, "Error: Invalid array size %d\n", size);
        return;
    }
    
    arrays[arr_idx].data = (int *)calloc(size, sizeof(int));
    if (!arrays[arr_idx].data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return;
    }
    
    arrays[arr_idx].size = size;
    arrays[arr_idx].allocated = true;
}

/* Execute INPUT statement */
void execute_input(void) {
    skip_whitespace();

    if (*current_pos == '"') {
        char *prompt = read_string_literal();
        if (prompt) {
            printf("%s", prompt);
            fflush(stdout);
        }

        skip_whitespace();
        if (*current_pos == ',') {
            current_pos++;
        }
    }

    while (1) {
        skip_whitespace();
        if (!*current_pos || *current_pos == '\n') {
            break;
        }

        if (!isalpha(*current_pos)) {
            fprintf(stderr, "Error: Expected variable name in INPUT\n");
            return;
        }

        char var_name = toupper(*current_pos);
        current_pos++;

        bool is_string = false;
        if (*current_pos == '$') {
            is_string = true;
            current_pos++;
        }

        int *target = NULL;

        if (is_string) {
            char buffer[MAX_LINE_LENGTH];
            if (scanf("%255s", buffer) == 1) { /* 255 must be MAX_LINE_LENGTH - 1 */
                int idx = var_name - 'A';
                if (string_variables[idx]) free(string_variables[idx]);
                string_variables[idx] = (char *)malloc(strlen(buffer) + 1);
                if (string_variables[idx]) strcpy(string_variables[idx], buffer);
            }
        } else {
            skip_whitespace();
            if (*current_pos == '[' || *current_pos == '(') {
                char closing = (*current_pos == '[') ? ']' : ')';
                current_pos++;
                int index = parse_expression();
                skip_whitespace();
                if (*current_pos == closing) {
                    current_pos++;
                }

                int arr_idx = var_name - 'A';
                if (!arrays[arr_idx].allocated) {
                    fprintf(stderr, "Error: Array %c not dimensioned\n", var_name);
                    return;
                }
                if (index < 0 || index >= arrays[arr_idx].size) {
                    fprintf(stderr, "Error: Array index %d out of bounds for %c\n", index, var_name);
                    return;
                }
                target = &arrays[arr_idx].data[index];
            } else {
                target = &variables[var_name - 'A'];
            }

            if (target) {
                if (scanf("%d", target) != 1) {
                    fprintf(stderr, "Error: Invalid input\n");
                    while(getchar() != '\n' && !feof(stdin));
                    return;
                }
            }
        }

        skip_whitespace();
        if (*current_pos == ',') {
            current_pos++;
        } else {
            break;
        }
    }

    /* Consume trailing newline */
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* Execute FOR statement */
void execute_for(void) {
    skip_whitespace();
    if (!isalpha(*current_pos)) {
        fprintf(stderr, "Error: Expected variable name in FOR\n");
        return;
    }
    char var_name = toupper(*current_pos);
    current_pos++;
    skip_whitespace();

    if (*current_pos == '=') {
        current_pos++;
    }

    int start_val = parse_expression();
    skip_whitespace();

    if (strncasecmp(current_pos, "TO", 2) != 0) {
        fprintf(stderr, "Error: Expected TO in FOR\n");
        return;
    }
    current_pos += 2;

    int end_val = parse_expression();
    skip_whitespace();

    int step_val = 1;
    if (strncasecmp(current_pos, "STEP", 4) == 0) {
        current_pos += 4;
        step_val = parse_expression();
    }

    /* Check if this loop is already on stack */
    if (for_stack_ptr > 0 && for_stack[for_stack_ptr - 1].line_index == current_line_index) {
        /* Re-entry from NEXT, variable already incremented */
    } else {
        /* Initial entry */
        if (for_stack_ptr >= MAX_FOR_STACK) {
            fprintf(stderr, "Error: FOR stack overflow\n");
            return;
        }
        variables[var_name - 'A'] = start_val;
        for_stack[for_stack_ptr].var_name = var_name;
        for_stack[for_stack_ptr].end_value = end_val;
        for_stack[for_stack_ptr].step_value = step_val;
        for_stack[for_stack_ptr].line_index = current_line_index;
        for_stack_ptr++;
    }

    /* Check condition */
    int current_val = variables[var_name - 'A'];
    bool done = false;
    if (step_val > 0 && current_val > end_val) done = true;
    else if (step_val < 0 && current_val < end_val) done = true;

    if (done) {
        for_stack_ptr--;
        skip_to_next(var_name);
    }
}

/* Execute NEXT statement */
void execute_next(void) {
    skip_whitespace();
    if (!isalpha(*current_pos)) {
        fprintf(stderr, "Error: Expected variable name in NEXT\n");
        return;
    }
    char var_name = toupper(*current_pos);
    current_pos++;

    if (for_stack_ptr > 0 && for_stack[for_stack_ptr - 1].var_name == var_name) {
        variables[var_name - 'A'] += for_stack[for_stack_ptr - 1].step_value;
        current_line_index = for_stack[for_stack_ptr - 1].line_index - 1;
    } else {
        fprintf(stderr, "Error: NEXT without matching FOR\n");
    }
}

/* Skip to matching NEXT statement */
void skip_to_next(char var_name) {
    int nesting = 0;
    while (current_line_index < program_size) {
        current_line_index++;
        if (current_line_index >= program_size) break;

        char *ptr = program[current_line_index].text;
        while (*ptr && isspace(*ptr)) ptr++;

        if (strncasecmp(ptr, "FOR", 3) == 0) {
            nesting++;
        } else if (strncasecmp(ptr, "NEXT", 4) == 0) {
            if (nesting == 0) {
                char *vptr = ptr + 4;
                while (*vptr && isspace(*vptr)) vptr++;
                if (toupper(*vptr) == var_name) {
                    return;
                }
            } else {
                nesting--;
            }
        }
    }
    fprintf(stderr, "Error: Matching NEXT %c not found\n", var_name);
}

/* Find line by line number */
int find_line(int line_number) {
    int i;
    for (i = 0; i < program_size; i++) {
        if (program[i].line_number == line_number) {
            return i;
        }
    }
    return -1;
}

/* Execute GOTO statement */
void execute_goto(void) {
    int line_num = parse_expression();
    int index = find_line(line_num);
    
    if (index >= 0) {
        current_line_index = index - 1; /* Will be incremented in run loop */
    } else {
        fprintf(stderr, "Error: Line %d not found\n", line_num);
    }
}

/* Parse string operand for comparison */
char *parse_string_operand(void) {
    skip_whitespace();
    if (*current_pos == '"') {
        char *s = read_string_literal();
        if (s) {
            char *ret = (char *)malloc(strlen(s) + 1);
            if (ret) strcpy(ret, s);
            return ret;
        }
    } else if (strncasecmp(current_pos, "LEFT$", 5) == 0) {
        current_pos += 5;
        skip_whitespace();
        if (*current_pos == '(') {
            current_pos++;
            char *str = parse_string_operand();
            skip_whitespace();
            if (*current_pos == ',') {
                current_pos++;
                int n = parse_expression();
                skip_whitespace();
                if (*current_pos == ')') {
                    current_pos++;
                    char *ret = NULL;
                    if (str) {
                        int len = strlen(str);
                        if (n < 0) n = 0;
                        if (n > len) n = len;
                        ret = (char *)malloc(n + 1);
                        if (ret) {
                            strncpy(ret, str, n);
                            ret[n] = '\0';
                        }
                        free(str);
                    }
                    return ret;
                }
            }
            if (str) free(str);
        }
    } else if (strncasecmp(current_pos, "RIGHT$", 6) == 0) {
        current_pos += 6;
        skip_whitespace();
        if (*current_pos == '(') {
            current_pos++;
            char *str = parse_string_operand();
            skip_whitespace();
            if (*current_pos == ',') {
                current_pos++;
                int n = parse_expression();
                skip_whitespace();
                if (*current_pos == ')') {
                    current_pos++;
                    char *ret = NULL;
                    if (str) {
                        int len = strlen(str);
                        if (n < 0) n = 0;
                        if (n > len) n = len;
                        ret = (char *)malloc(n + 1);
                        if (ret) {
                            strcpy(ret, str + (len - n));
                        }
                        free(str);
                    }
                    return ret;
                }
            }
            if (str) free(str);
        }
    } else if (strncasecmp(current_pos, "MID$", 4) == 0) {
        current_pos += 4;
        skip_whitespace();
        if (*current_pos == '(') {
            current_pos++;
            char *str = parse_string_operand();
            skip_whitespace();
            if (*current_pos == ',') {
                current_pos++;
                int start = parse_expression();
                skip_whitespace();
                if (*current_pos == ',') {
                    current_pos++;
                    int n = parse_expression();
                    skip_whitespace();
                    if (*current_pos == ')') {
                        current_pos++;
                        char *ret = NULL;
                        if (str) {
                            int len = strlen(str);
                            if (start < 1) start = 1;
                            if (start > len) {
                                ret = (char *)malloc(1);
                                if (ret) *ret = '\0';
                            } else {
                                int available = len - (start - 1);
                                if (n < 0) n = 0;
                                if (n > available) n = available;
                                ret = (char *)malloc(n + 1);
                                if (ret) {
                                    strncpy(ret, str + (start - 1), n);
                                    ret[n] = '\0';
                                }
                            }
                            free(str);
                        }
                        return ret;
                    }
                }
            }
            if (str) free(str);
        }
    } else if (isalpha(*current_pos)) {
         char *save_pos = current_pos;
         char var = toupper(*current_pos++);
         if (*current_pos == '$') {
             current_pos++; /* skip $ */
             int idx = var - 'A';
             char *val = string_variables[idx];
             if (val) {
                 char *ret = (char *)malloc(strlen(val) + 1);
                 if (ret) strcpy(ret, val);
                 return ret;
             } else {
                 char *ret = (char *)malloc(1);
                 if (ret) *ret = '\0';
                 return ret;
             }
         }
         current_pos = save_pos; /* backtrack */
    }
    return NULL;
}

/* Execute IF statement */
void execute_if(void) {
    skip_whitespace();

    char *left_str = NULL;
    char *right_str = NULL;
    bool is_string_comp = false;

    int left_val = 0;

    char *save_pos = current_pos;
    left_str = parse_string_operand();

    if (left_str) {
        is_string_comp = true;
    } else {
        current_pos = save_pos;
        left_val = parse_expression();
    }

    skip_whitespace();
    
    char op[3] = {0};
    int op_len = 0;
    
    /* Read comparison operator */
    if (*current_pos == '=' || *current_pos == '<' || *current_pos == '>') {
        op[op_len++] = *current_pos++;
        if (*current_pos == '=' || (*current_pos == '>' && op[0] == '<')) {
            op[op_len++] = *current_pos++;
        }
    }
    
    int right_val = 0;

    if (is_string_comp) {
        right_str = parse_string_operand();
        if (!right_str) {
            fprintf(stderr, "Error: Type mismatch in IF\n");
            if (left_str) free(left_str);
            return;
        }
    } else {
        right_val = parse_expression();
    }

    bool condition = false;
    
    if (is_string_comp) {
        int cmp = strcmp(left_str, right_str);
        if (strcmp(op, "=") == 0 || strcmp(op, "==") == 0) {
            condition = (cmp == 0);
        } else if (strcmp(op, "<") == 0) {
            condition = (cmp < 0);
        } else if (strcmp(op, ">") == 0) {
            condition = (cmp > 0);
        } else if (strcmp(op, "<=") == 0) {
            condition = (cmp <= 0);
        } else if (strcmp(op, ">=") == 0) {
            condition = (cmp >= 0);
        } else if (strcmp(op, "<>") == 0 || strcmp(op, "!=") == 0) {
            condition = (cmp != 0);
        }

        free(left_str);
        free(right_str);
    } else {
        /* Evaluate integer condition */
        if (strcmp(op, "=") == 0 || strcmp(op, "==") == 0) {
            condition = (left_val == right_val);
        } else if (strcmp(op, "<") == 0) {
            condition = (left_val < right_val);
        } else if (strcmp(op, ">") == 0) {
            condition = (left_val > right_val);
        } else if (strcmp(op, "<=") == 0) {
            condition = (left_val <= right_val);
        } else if (strcmp(op, ">=") == 0) {
            condition = (left_val >= right_val);
        } else if (strcmp(op, "<>") == 0 || strcmp(op, "!=") == 0) {
            condition = (left_val != right_val);
        }
    }
    
    /* Look for THEN or GOTO */
    skip_whitespace();
    if (strncasecmp(current_pos, "THEN", 4) == 0) {
        current_pos += 4;
        skip_whitespace();
    }
    
    if (condition) {
        if (strncasecmp(current_pos, "GOTO", 4) == 0) {
            current_pos += 4;
            execute_goto();
        } else if (strncasecmp(current_pos, "PRINT", 5) == 0) {
            current_pos += 5;
            execute_print();
        } else if (strncasecmp(current_pos, "LET", 3) == 0) {
            current_pos += 3;
            execute_let();
        } else if (isalpha(*current_pos)) {
            /* Direct assignment without LET */
            execute_let();
        }
    }
}

/* Execute a single line */
void execute_line(int line_index) {
    char *line = program[line_index].text;
    current_pos = line;
    
    skip_whitespace();
    
    if (strncasecmp(current_pos, "PRINT", 5) == 0) {
        current_pos += 5;
        execute_print();
    } else if (strncasecmp(current_pos, "LET", 3) == 0) {
        current_pos += 3;
        execute_let();
    } else if (strncasecmp(current_pos, "GOTO", 4) == 0) {
        current_pos += 4;
        execute_goto();
    } else if (strncasecmp(current_pos, "IF", 2) == 0) {
        current_pos += 2;
        execute_if();
    } else if (strncasecmp(current_pos, "DIM", 3) == 0) {
        current_pos += 3;
        execute_dim();
    } else if (strncasecmp(current_pos, "INPUT", 5) == 0) {
        current_pos += 5;
        execute_input();
    } else if (strncasecmp(current_pos, "FOR", 3) == 0) {
        current_pos += 3;
        execute_for();
    } else if (strncasecmp(current_pos, "NEXT", 4) == 0) {
        current_pos += 4;
        execute_next();
    } else if (strncasecmp(current_pos, "END", 3) == 0) {
        current_line_index = program_size; /* Exit program */
    } else if (*current_pos) {
        /* Assume it's a LET statement without LET keyword */
        execute_let();
    }
}

/* Run the program */
void run_program(void) {
    if (program_size == 0) {
        printf("No program to run.\n");
        return;
    }
    
    for (current_line_index = 0; current_line_index < program_size; current_line_index++) {
        execute_line(current_line_index);
    }
}

/* List the program */
void list_program(void) {
    int i;
    for (i = 0; i < program_size; i++) {
        printf("%d %s\n", program[i].line_number, program[i].text);
    }
}

/* Clear the program */
void clear_program(void) {
    program_size = 0;
    init_interpreter();
}

/* Insert or replace a line in the program */
void insert_line(int line_number, const char *text) {
    int i;
    
    /* Find insertion point */
    int insert_pos = 0;
    for (i = 0; i < program_size; i++) {
        if (program[i].line_number == line_number) {
            /* Replace existing line */
            if (strlen(text) == 0) {
                /* Delete line */
                for (int j = i; j < program_size - 1; j++) {
                    program[j] = program[j + 1];
                }
                program_size--;
            } else {
                strncpy(program[i].text, text, MAX_LINE_LENGTH - 1);
                program[i].text[MAX_LINE_LENGTH - 1] = '\0';
            }
            return;
        }
        if (program[i].line_number < line_number) {
            insert_pos = i + 1;
        }
    }
    
    /* Insert new line */
    if (strlen(text) > 0 && program_size < MAX_LINES) {
        for (i = program_size; i > insert_pos; i--) {
            program[i] = program[i - 1];
        }
        program[insert_pos].line_number = line_number;
        strncpy(program[insert_pos].text, text, MAX_LINE_LENGTH - 1);
        program[insert_pos].text[MAX_LINE_LENGTH - 1] = '\0';
        program_size++;
    }
}

/* Save program to file */
void save_program(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file %s for writing\n", filename);
        return;
    }
    
    int i;
    for (i = 0; i < program_size; i++) {
        fprintf(fp, "%d %s\n", program[i].line_number, program[i].text);
    }
    
    fclose(fp);
    printf("Program saved to %s\n", filename);
}

/* Load program from file */
void load_program(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file %s for reading\n", filename);
        return;
    }
    
    clear_program();
    
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), fp)) {
        int line_num;
        char rest[MAX_LINE_LENGTH];
        
        /* Remove newline */
        line[strcspn(line, "\n")] = '\0';
        
        if (sscanf(line, "%d %[^\n]", &line_num, rest) == 2) {
            insert_line(line_num, rest);
        }
    }
    
    fclose(fp);
    printf("Program loaded from %s\n", filename);
}

/* Main interpreter loop */
int main(void) {
    char input[MAX_LINE_LENGTH];
    
    init_interpreter();
    
    printf("Tiny BASIC Interpreter\n");
    printf("Commands: NEW, LIST, RUN, LOAD <file>, SAVE <file>, QUIT\n");
    printf("Statements: PRINT, LET, GOTO, IF, DIM, END\n\n");
    
    while (1) {
        printf("> ");
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        /* Remove newline */
        input[strcspn(input, "\n")] = '\0';
        
        /* Skip empty lines */
        if (strlen(input) == 0) {
            continue;
        }
        
        /* Check for commands */
        if (strcasecmp(input, "QUIT") == 0) {
            break;
        } else if (strcasecmp(input, "NEW") == 0) {
            clear_program();
            printf("Program cleared.\n");
        } else if (strcasecmp(input, "LIST") == 0) {
            list_program();
        } else if (strcasecmp(input, "RUN") == 0) {
            run_program();
        } else if (strncasecmp(input, "LOAD ", 5) == 0) {
            load_program(input + 5);
        } else if (strncasecmp(input, "SAVE ", 5) == 0) {
            save_program(input + 5);
        } else {
            /* Check if it's a numbered line */
            int line_num;
            char rest[MAX_LINE_LENGTH];
            
            if (sscanf(input, "%d %[^\n]", &line_num, rest) == 2) {
                insert_line(line_num, rest);
            } else if (sscanf(input, "%d", &line_num) == 1) {
                /* Line number with no text - delete the line */
                insert_line(line_num, "");
            } else {
                /* Direct execution of statement */
                current_pos = input;
                current_line_index = 0;
                
                if (strncasecmp(current_pos, "PRINT", 5) == 0) {
                    current_pos += 5;
                    execute_print();
                } else if (strncasecmp(current_pos, "LET", 3) == 0) {
                    current_pos += 3;
                    execute_let();
                } else if (strncasecmp(current_pos, "DIM", 3) == 0) {
                    current_pos += 3;
                    execute_dim();
                } else if (strncasecmp(current_pos, "INPUT", 5) == 0) {
                    current_pos += 5;
                    execute_input();
                } else if (strncasecmp(current_pos, "FOR", 3) == 0) {
                    current_pos += 3;
                    execute_for();
                } else if (strncasecmp(current_pos, "NEXT", 4) == 0) {
                    current_pos += 4;
                    execute_next();
                } else {
                    printf("Unknown command or invalid syntax\n");
                }
            }
        }
    }
    
    cleanup_interpreter();
    printf("Goodbye!\n");
    return 0;
}
