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

#include <sapi/embed/php_embed.h>
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

/*
 * Declare ourselves so the configuration routines can find and know us.
 * We'll fill it in at the end of the module.
 */
mmodule php_module;

typedef struct
{
    zval* phpmod;     /* The php metric module object */
    zval* phpcb;      /* The metric call back function */
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
    zval* phpcb;
}
php_metric_init_t;

static apr_pool_t *pool;

static apr_array_header_t *metric_info = NULL;
static apr_array_header_t *metric_mapping_info = NULL;
static apr_status_t php_metric_cleanup ( void *data);

char modname_bfr[PATH_MAX];
static char* is_php_module(const char* fname)
{
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
}

static void fill_gmi(Ganglia_25metric* gmi, php_metric_init_t* minfo)
{
	/* TODO */
}

static cfg_t* find_module_config(char *modname)
{
    cfg_t *modules_cfg;
    int j;

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

static HashTable* build_params_dict(cfg_t *phpmodule)
{
	/* TODO */
}

static PhpMethodDef GangliaMethods[] = {
    {"get_debug_msg_level", ganglia_get_debug_msg_level, METH_NOARGS,
     "Return the debug level used by ganglia."},
    {NULL, NULL, 0, NULL}
};

static int php_metric_init (apr_pool_t *p)
{
	/* TODO */
}

static apr_status_t php_metric_cleanup ( void *data)
{
	/* TODO */
}

static g_val_t php_metric_handler( int metric_index )
{
	/* TODO */
}

mmodule php_module =
{
    STD_MMODULE_STUFF,
    php_metric_init,
    NULL,
    NULL, /* defined dynamically */
    php_metric_handler,
};
