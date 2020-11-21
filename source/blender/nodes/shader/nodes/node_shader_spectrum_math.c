/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2005 Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup shdnodes
 */

#include "node_shader_util.h"

static bNodeSocketTemplate sh_node_spectrum_math_in[] = {
    {SOCK_RGBA, N_("Spectrum"), 0.5f, 0.5f, 0.5f, 1.0f},
    {SOCK_RGBA, N_("Spectrum"), 0.5f, 0.5f, 0.5f, 1.0f},
    {-1, ""}};

static bNodeSocketTemplate sh_node_spectrum_math_out[] = {{SOCK_RGBA, N_("Spectrum")}, {-1, ""}};

void register_node_type_sh_spectrum_math(void)
{
  static bNodeType ntype;

  sh_node_type_base(&ntype, SH_NODE_SPECTRUM_MATH, "Spectrum Math", NODE_CLASS_CONVERTOR, 0);
  node_type_socket_templates(&ntype, sh_node_spectrum_math_in, sh_node_spectrum_math_out);
  node_type_label(&ntype, node_spectrum_math_label);
  node_type_update(&ntype, node_spectrum_math_update);

  nodeRegisterType(&ntype);
}
