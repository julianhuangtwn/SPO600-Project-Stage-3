// Auto generated during GCC build process
#include "config.h"

// System definitions and utilities
#include "system.h"

// Defines core types used in GCC, essential for working with data structures
#include "coretypes.h"

// For basic_bock and FOR_EACH_BB_FN
#include "basic-block.h"

// Defines tree data structure, GCC's representation for expressions and types
#include "tree.h"

// Defines GCC pass infrastructure, including opt_pass
#include "tree-pass.h"

// For `struct function`
#include "function.h"

// For GIMPLE statements
#include "gimple.h"

// For gimple_stmt_iterator
#include "gimple-iterator.h"

// For control flow graph
#include "cfg.h"

// Vector
#include "vec.h"

// For dump file
#include "diagnostic-core.h"

// Structure to store function information 
struct function_info {
    const char *base_name;
    unsigned hash;
};

// Vector to store function hashes 
static vec<function_info> function_hashes;

// Pass definition 
const pass_data prj_pass_data = {
    GIMPLE_PASS, /* type */
    "prj_pass", /* name */
    OPTGROUP_NONE, /* optinfo_flags */
    TV_NONE, /* tv_id */
    PROP_gimple_any, /* properties_required */
    PROP_gimple_any, /* properties_provided */
    0, /* properties_destroyed */
    0, /* todo_flags_start */
    0, /* todo_flags_finish */
};

// Checks if any clone ends with resolver
bool is_internal_clone(const char* name) {
    return strstr(name, "resolver") != NULL;
}

// Checks if function is a clone variant
static bool is_related_function(const char *name, char **base_name) {
    // strrchr finds last occurrence of '.'
    const char *dot = strrchr(name, '.');

    // Return true if no '.' as original base function
    if (!dot) {
        // Duplicates the name back into base_name
        *base_name = xstrdup(name);
        return true;
    }
    
    // Shift pointer to first character after '.' to get suffix
    const char *suffix = dot + 1;

    // We want any function with a . for extension, but not compiler transformations such as .resolver
    if (strlen(suffix) > 0 && !is_internal_clone(suffix)) { 
        // Duplicates the name back into base_name from name to dot - name (without suffix)
        *base_name = xstrndup(name, dot - name);
        return true;
    }
    
    return false;
}

// Generate a hash for the function's structure to compare original and clone
static unsigned generate_function_hash(function *fun) {
    // Initial seed value for the DJB2 hash algorithm
    unsigned hash = 5381;

    // Basic block structure of the function
    basic_block bb;
    
    // fun-decl contains the function's name and return type
    // TREE_TYPE gets the type of the function
    // Calling twice allows us to get the return type
    tree ret_type = TREE_TYPE(TREE_TYPE(fun->decl));

    // DJB2 hash algorithm
    // << is left shift
    // Hash and add the return type if it exists otherwise 0
    hash = ((hash << 5) + hash) + (ret_type ? TREE_CODE(ret_type) : 0);
    
    // DECL_ARGUMENTS gets the function's parameters
    tree params = DECL_ARGUMENTS(fun->decl);

    // Go through all parameters
    while (params) {
        // Get the single parameter
        tree param_type = TREE_TYPE(params);
        // Add the parameter type if it exists otherwise 0
        hash = ((hash << 5) + hash) + (param_type ? TREE_CODE(param_type) : 0);
        // TREE_CHAIN gets the next parameter
        params = TREE_CHAIN(params);
    }
    
    // Go through the function's basic blocks just like previous stage
    FOR_EACH_BB_FN(bb, fun) {
        // Get the last statement in the block
        gimple_stmt_iterator gsi = gsi_last_bb(bb);

        // If gsi iterator is not at the end
        if (!gsi_end_p(gsi)) {
            // Get the statement
            gimple *last = gsi_stmt(gsi);
            // Add the code of the statement
            hash = ((hash << 5) + hash) + gimple_code(last);
        }
        
        // Count statements in block 
        unsigned stmt_count = 0;

        // Iterate through the blocks using gsi iterator
        for (gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
            stmt_count++;
        }
        hash = ((hash << 5) + hash) + stmt_count;
    }

    return hash;
}

// Main pass execution function 
static unsigned int prj_pass_execute(function *fun) {
    // We don't want anything without a function to run this or it will crash
    // We check if function, function declaration, and function name exist
    if (!fun || !fun->decl || !DECL_NAME(fun->decl)) {
        return 0;
    }

    // Get the name of the function
    const char *name = IDENTIFIER_POINTER(DECL_NAME(fun->decl));
    
    
    char *base_name = NULL;
    // Check if the function is a clone
    if (!is_related_function(name, &base_name)) {
        // Free memory and exit if not
        free(base_name);
        return 0;
    }
    
    // Generate hash for the function
    unsigned hash = generate_function_hash(fun);
    // Duoplicate the base name as a permanent key
    const char *persistent_key = ggc_strdup(base_name);
    free(base_name);
    
    // Search existing hashes
    bool found = false;
    unsigned i;
    function_info *fi;
    
    // Iterate through existing hashes
    FOR_EACH_VEC_ELT (function_hashes, i, fi) {
        if (strcmp(fi->base_name, persistent_key) == 0) {
            found = true;
            // If hash matches
            if (fi->hash == hash) {
                //fprintf(stderr, "PRUNE: %s\n", name);
                inform(DECL_SOURCE_LOCATION(fun->decl), "PRUNE: %s", name);
            } else {
                //fprintf(stderr, "NOPRUNE: %s\n", name);
                inform(DECL_SOURCE_LOCATION(fun->decl), "NOPRUNE: %s", name);
            }
            break;
        }
    }
    
    // If not found add new function hash to the vector
    if (!found) {
        function_info new_func = {persistent_key, hash};
        function_hashes.safe_push(new_func);
    }
    
    return 0;
}

// Pass initialization require to create the vector first
static void init_prj_pass(void) {
    function_hashes.create(0);
}

class prj_pass : public gimple_opt_pass {
public:
    // Pass constructor 
    prj_pass(gcc::context *ctxt) : gimple_opt_pass(prj_pass_data, ctxt) {}
    
    unsigned int execute(function *fun) override { 
        return prj_pass_execute(fun); 
    }
    
    // Required by GCC pass even if unused in this case
    void set_pass_param(unsigned int, bool) override {}
    
    // Activate pass
    bool gate(function *) override { 
        return true; 
    }
};

// Pass creation 
opt_pass *make_prj_pass(gcc::context *ctxt) {
    init_prj_pass();
    return new prj_pass(ctxt);
}

