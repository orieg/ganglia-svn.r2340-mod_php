/*******************************************************************************
*
* This code is part of a php module for ganglia.
*
* Author : Nicolas Brousse (nicolas brousse.info)
*
* Portions Copyright (C) 2007 Novell, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  - Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
*  - Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
*  - Neither the name of Novell, Inc. nor the names of its
*    contributors may be used to endorse or promote products derived from this
*    software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL Novell, Inc. OR THE CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#include <gm_metric.h>
#include <gm_msg.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "file.h"

#include <apr_tables.h>
#include <apr_strings.h>
#include <apr_lib.h>

#include <dirent.h>
#include <sys/stat.h>

#include <sapi/embed/php_embed.h>

#ifdef ZTS
	void ***tsrm_ls;
#endif

#define php_verbose_debug(debug_level, ...) { \
	if (get_debug_msg_level() > debug_level) { \
	   debug_msg(__VA_ARGS__); \
	} \
}

/*
 * Declare ourselves so the configuration routines can find and know us.
 * We'll fill it in at the end of the module.
 */
mmodule php_module;

typedef struct
{
    zval* phpmod;     /* The php metric module object */
    zval* callback;   /* The metric call back function */
    char *mod_name;   /* The name */
}
mapped_info_t;

typedef struct
{
    char mname[128];
    int tmax;
    char vtype[32];
    char units[64];
    char slope[32];
    char format[64];
    char desc[512];
    char groups[512];
    apr_table_t *extra_data;
    zval* callback;
}
php_metric_init_t;

static apr_pool_t *pool;

static apr_array_header_t *metric_info = NULL;
static apr_array_header_t *metric_mapping_info = NULL;
static apr_status_t php_metric_cleanup ( void *data);

char modname_bfr[PATH_MAX];
static char* is_php_module(const char* fname)
{
	php_verbose_debug(3, "is_php_module");
    char* p = strrchr(fname, '.');
    if (!p) {
        return NULL;
    }

    if (strcmp(p, ".php")) {
        return NULL;
    }

    strncpy(modname_bfr, fname, p-fname);
    modname_bfr[p-fname] = 0;
    return modname_bfr;
}

static void fill_metric_info(HashTable* ht, php_metric_init_t* minfo, char *modname, apr_pool_t *pool)
{
	/* TODO */
	php_verbose_debug(3, "fill_metric_info");
}

static void fill_gmi(Ganglia_25metric* gmi, php_metric_init_t* minfo)
{
    char *s, *lasts;
    int i;
    const apr_array_header_t *arr = apr_table_elts(minfo->extra_data);
    const apr_table_entry_t *elts = (const apr_table_entry_t *)arr->elts;

    php_verbose_debug(3, "fill_gmi");

    /* gmi->key will be automatically assigned by gmond */
    gmi->name = apr_pstrdup (pool, minfo->mname);
    gmi->tmax = minfo->tmax;
    if (!strcasecmp(minfo->vtype, "string")) {
        gmi->type = GANGLIA_VALUE_STRING;
        gmi->msg_size = UDP_HEADER_SIZE+MAX_G_STRING_SIZE;
    }
    else if (!strcasecmp(minfo->vtype, "uint")) {
        gmi->type = GANGLIA_VALUE_UNSIGNED_INT;
        gmi->msg_size = UDP_HEADER_SIZE+8;
    }
    else if (!strcasecmp(minfo->vtype, "int")) {
        gmi->type = GANGLIA_VALUE_INT;
        gmi->msg_size = UDP_HEADER_SIZE+8;
    }
    else if (!strcasecmp(minfo->vtype, "float")) {
        gmi->type = GANGLIA_VALUE_FLOAT;
        gmi->msg_size = UDP_HEADER_SIZE+8;
    }
    else if (!strcasecmp(minfo->vtype, "double")) {
        gmi->type = GANGLIA_VALUE_DOUBLE;
        gmi->msg_size = UDP_HEADER_SIZE+16;
    }
    else {
        gmi->type = GANGLIA_VALUE_UNKNOWN;
        gmi->msg_size = UDP_HEADER_SIZE+8;
    }

    gmi->units = apr_pstrdup(pool, minfo->units);
    gmi->slope = apr_pstrdup(pool, minfo->slope);
    gmi->fmt = apr_pstrdup(pool, minfo->format);
    gmi->desc = apr_pstrdup(pool, minfo->desc);

    MMETRIC_INIT_METADATA(gmi, pool);
    for (s=(char *)apr_strtok(minfo->groups, ",", &lasts);
          s!=NULL; s=(char *)apr_strtok(NULL, ",", &lasts)) {
        char *d = s;
        /* Strip the leading white space */
        while (d && *d && apr_isspace(*d)) {
            d++;
        }
        MMETRIC_ADD_METADATA(gmi,MGROUP,d);
    }

    /* transfer any extra data as metric metadata */
    for (i = 0; i < arr->nelts; ++i) {
        if (elts[i].key == NULL)
            continue;
        MMETRIC_ADD_METADATA(gmi, elts[i].key, elts[i].val);
    }
}

static cfg_t* find_module_config(char *modname)
{
    cfg_t *modules_cfg;
    int j;

    php_verbose_debug(3, "find_module_config");

    modules_cfg = cfg_getsec(php_module.config_file, "modules");
    for (j = 0; j < cfg_size(modules_cfg, "module"); j++) {
        char *modName, *modLanguage;
        int modEnabled;

        cfg_t *phpmodule = cfg_getnsec(modules_cfg, "module", j);

        /* Check the module language to make sure that
           the language designation is PHP.
        */
        modLanguage = cfg_getstr(phpmodule, "language");
        if (!modLanguage || strcasecmp(modLanguage, "php"))
            continue;

        modName = cfg_getstr(phpmodule, "name");
        if (strcasecmp(modname, modName)) {
            continue;
        }

        /* Check to make sure that the module is enabled.
        */
        modEnabled = cfg_getbool(phpmodule, "enabled");
        if (!modEnabled)
            continue;

        return phpmodule;
    }
    return NULL;
}

static zval* build_params_dict(cfg_t *phpmodule)
{
    int k;
    zval *params_dict;

    /* Create params_dict as a ZVAL ARRAY */
    MAKE_STD_ZVAL(params_dict);
    array_init(params_dict);

    if (phpmodule) {
        for (k = 0; k < cfg_size(phpmodule, "param"); k++) {
            cfg_t *param;
            char *name, *value;

            param = cfg_getnsec(phpmodule, "param", k);
            name = apr_pstrdup(pool, param->title);
            value = apr_pstrdup(pool, cfg_getstr(param, "value"));
            if (name && value) {
            	add_assoc_string(params_dict, name, value, 1);
            }
        }
    }

    return params_dict;
}

static int php_metric_init (apr_pool_t *p)
{
	php_verbose_debug(3, "php_metric_init");

	DIR *dp;
	struct dirent *entry;
	int i;
	char* modname;
	zval retval, funcname, *params, *type, **server;
	zend_uint params_length;
	php_metric_init_t minfo;
	Ganglia_25metric *gmi;
	mapped_info_t *mi;
	const char* path = php_module.module_params;
	cfg_t *module_cfg;

	zend_file_handle script;

	php_verbose_debug(2, "php_modules path: %s", path);

	/* Allocate a pool that will be used by this module */
	apr_pool_create(&pool, p);

	metric_info = apr_array_make(pool, 10, sizeof(Ganglia_25metric));
	metric_mapping_info = apr_array_make(pool, 10, sizeof(mapped_info_t));

	/* Verify path exists and can be read */

	if (!path) {
		err_msg("[PHP] Missing php modules path.\n");
		return -1;
	}

    if (access(path, F_OK)) {
        /* 'path' does not exist */
        err_msg("[PHP] Can't open the PHP module path %s.\n", path);
        return -1;
    }

    if (access(path, R_OK)) {
        /* Don't have read access to 'path' */
        err_msg("[PHP] Can't read from the PHP module path %s.\n", path);
        return -1;
    }

    /* Initialize each perl module */
    if ((dp = opendir(path)) == NULL) {
        /* Error: Cannot open the directory - Shouldn't happen */
        /* Log? */
        err_msg("[PHP] Can't open the PHP module path %s.\n", path);
        return -1;
    }

	php_embed_init(0, NULL PTSRMLS_CC);
	php_verbose_debug(3, "php_embed_init");

	/* Fetch $_SERVER from the global scope */
	zend_hash_find(&EG(symbol_table), "_SERVER", sizeof("_SERVER"), (void**) &server);

	/* $_SERVER['SAPI_TYPE'] = 'embed' */
	ALLOC_INIT_ZVAL(type);
	ZVAL_STRING(type, "embed", 1);
	ZEND_SET_SYMBOL(Z_ARRVAL_PP(server), "SAPI_TYPE", type);

    i = 0;

    while ((entry = readdir(dp)) != NULL) {
        modname = is_php_module(entry->d_name);

        if (modname == NULL)
            continue;

        /* Find the specified module configuration in gmond.conf
           If this return NULL then either the module config
           doesn't exist or the module is disabled. */
        module_cfg = find_module_config(modname);
        if (!module_cfg)
            continue;

        char file[256];
        strcpy(file, path);
        strcat(file, "/");
        strcat(file, modname);
        strcat(file, ".php");

        script.type = ZEND_HANDLE_FP;
        script.filename = file;
        script.opened_path = NULL;
        script.free_filename = 0;
        if (!(script.handle.fp = fopen(script.filename, "rb"))) {
        	err_msg("Unable to open %s\n", script.filename);
        	continue;
        }

        php_execute_script(&script TSRMLS_CC);

        /* Build a parameter dictionary to pass to the module */
        params = build_params_dict(module_cfg);
        if (!params || Z_TYPE_P(params) != IS_ARRAY) {
            /* No metric_init function. */
            err_msg("[PHP] Can't build the parameters array for [%s].\n", modname);
            continue;
        }
        php_verbose_debug(3, "built the parameters dictionnary for the php module [%s]", modname);

        /* Now call the metric_init method of the python module */
        ZVAL_STRING(&funcname,"metric_init", 0);
        params_length = zend_hash_num_elements(Z_ARRVAL_P(params));
        if (call_user_function(EG(function_table), NULL, &funcname, &retval,
        		params_length, &params TSRMLS_CC) == FAILURE) {
        	/* failed calling metric_init */
            err_msg("[PHP] Can't call the metric_init function in the php module [%s]\n", modname);
            continue;
        }
        php_verbose_debug(3, "called the metric_init function for the php module [%s]\n", modname);

        if (Z_TYPE_P(&retval) == IS_ARRAY) {
            /*int j;
            int size = PyList_Size(pobj);
            for (j = 0; j < size; j++) {
                PyObject* plobj = PyList_GetItem(pobj, j);
                if (PyMapping_Check(plobj)) {
                    fill_metric_info(plobj, &minfo, modname, pool);
                    gmi = (Ganglia_25metric*)apr_array_push(metric_info);
                    fill_gmi(gmi, &minfo);
                    mi = (mapped_info_t*)apr_array_push(metric_mapping_info);
                    mi->pmod = pmod;
                    mi->mod_name = apr_pstrdup(pool, modname);
                    mi->pcb = minfo.pcb;
                }
            }*/
        }
        /*else if (PyMapping_Check(pobj)) {
            fill_metric_info(pobj, &minfo, modname, pool);
            gmi = (Ganglia_25metric*)apr_array_push(metric_info);
            fill_gmi(gmi, &minfo);
            mi = (mapped_info_t*)apr_array_push(metric_mapping_info);
            mi->pmod = pmod;
            mi->mod_name = apr_pstrdup(pool, modname);
            mi->pcb = minfo.pcb;
        }
        Py_DECREF(pobj);
        Py_DECREF(pinitfunc);
        gtstate = PyEval_SaveThread();
*/
    }
    closedir(dp);

    apr_pool_cleanup_register(pool, NULL,
                              php_metric_cleanup,
                              apr_pool_cleanup_null);

    /* Replace the empty static metric definition array with the
       dynamic array that we just created
    */
    gmi = apr_array_push(metric_info);
    memset (gmi, 0, sizeof(*gmi));
    mi = apr_array_push(metric_mapping_info);
    memset (mi, 0, sizeof(*mi));

    php_module.metrics_info = (Ganglia_25metric *)metric_info->elts;
	return 0;
}

static apr_status_t php_metric_cleanup ( void *data)
{
	php_verbose_debug(3, "php_metric_cleanup");
	php_embed_shutdown(TSRMLS_C);

	/* TODO */
    return APR_SUCCESS;
}

static g_val_t php_metric_handler( int metric_index )
{
    g_val_t val;
    Ganglia_25metric *gmi = (Ganglia_25metric *) metric_info->elts;
    mapped_info_t *mi = (mapped_info_t*) metric_mapping_info->elts;

    php_verbose_debug(3, "php_metric_handler");

    memset(&val, 0, sizeof(val));
    if (!mi[metric_index].callback) {
        /* No call back provided for this metric */
        return val;
    }

	/* TODO */
    return val;
}

mmodule php_module =
{
    STD_MMODULE_STUFF,
    php_metric_init,
    NULL,
    NULL, /* defined dynamically */
    php_metric_handler,
};
