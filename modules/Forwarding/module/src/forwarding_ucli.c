/****************************************************************
 * 
 *        Copyright 2013, Big Switch Networks, Inc. 
 * 
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * 
 *        http://www.eclipse.org/legal/epl-v10.html
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 * 
 ***************************************************************/

#include <indigo/types.h>
#include <Forwarding/forwarding_config.h>


#if FORWARDING_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>



static ucli_status_t
forwarding_ucli_ucli__config__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc,
                      "config", 0,
                      "$summary#config command.");
    ucli_printf(uc, "todo: support config command\n");

    return UCLI_STATUS_OK; 
}

static ucli_status_t
forwarding_ucli_ucli__foo__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc,
                      "foo", 0,
                      "$summary#test command.");
    ucli_printf(uc, "foo command issued\n");

    return UCLI_STATUS_OK; 
}


/* <auto.ucli.handlers.start> */
/******************************************************************************
 *
 * These handler table(s) were autogenerated from the symbols in this 
 * source file. 
 *
 *****************************************************************************/
static ucli_command_handler_f forwarding_ucli_ucli_handlers__[] = 
{
    forwarding_ucli_ucli__config__,
    forwarding_ucli_ucli__foo__,
    NULL
};
/******************************************************************************/
/* <auto.ucli.handlers.end> */

static ucli_module_t
forwarding_ucli_module__ = 
    {
        "forwarding_ucli", 
        NULL, 
        forwarding_ucli_ucli_handlers__, 
        NULL, 
        NULL, 
    }; 


ucli_node_t*
forwarding_ucli_node_create(void)
{
    ucli_node_t* n; 
    ucli_module_init(&forwarding_ucli_module__); 
    n = ucli_node_create("forwarding", NULL,
                         &forwarding_ucli_module__); 
    ucli_node_subnode_add(n,
                          ucli_module_log_node_create("forwarding"));
    return n;
}

#else
void*
forwarding_ucli_node_create(void)
{
    return NULL; 
}
#endif
