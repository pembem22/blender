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

#include "../node_shader_util.h"
#include "sky_model.h"

/* **************** OUTPUT ******************** */

static bNodeSocketTemplate sh_node_tex_sky_spectral_in[] = {
    {SOCK_VECTOR, N_("Vector"), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, PROP_NONE, SOCK_HIDE_VALUE},
    {-1, ""},
};

static bNodeSocketTemplate sh_node_tex_sky_spectral_out[] = {
    {SOCK_RGBA,
     N_("Spectrum"),
     0.0f,
     0.0f,
     0.0f,
     0.0f,
     0.0f,
     1.0f,
     PROP_NONE,
     SOCK_NO_INTERNAL_LINK},
    {-1, ""},
};

static void node_shader_init_tex_sky_spectral(bNodeTree *UNUSED(ntree), bNode *node)
{
  NodeTexSky *tex = MEM_callocN(sizeof(NodeTexSky), "NodeTexSky");
  BKE_texture_mapping_default(&tex->base.tex_mapping, TEXMAP_TYPE_POINT);
  BKE_texture_colormapping_default(&tex->base.color_mapping);
  tex->sun_disc = true;
  tex->sun_size = DEG2RADF(0.545);
  tex->sun_intensity = 1.0f;
  tex->sun_elevation = DEG2RADF(15.0f);
  tex->sun_rotation = 0.0f;
  tex->altitude = 0.0f;
  tex->air_density = 1.0f;
  tex->dust_density = 1.0f;
  tex->ozone_density = 1.0f;
  node->storage = tex;
}

static void node_shader_update_sky_spectral(bNodeTree *UNUSED(ntree), bNode *node)
{
  bNodeSocket *sockVector = nodeFindSocket(node, SOCK_IN, "Vector");

  nodeSetSocketAvailability(sockVector, false);
}

/* node type definition */
void register_node_type_sh_tex_sky_spectral(void)
{
  static bNodeType ntype;

  sh_node_type_base(
      &ntype, SH_NODE_TEX_SKY_SPECTRAL, "Nishita Spectral Sky Texture", NODE_CLASS_TEXTURE, 0);
  node_type_socket_templates(&ntype, sh_node_tex_sky_spectral_in, sh_node_tex_sky_spectral_out);
  node_type_size_preset(&ntype, NODE_SIZE_MIDDLE);
  node_type_init(&ntype, node_shader_init_tex_sky_spectral);
  node_type_storage(&ntype, "NodeTexSky", node_free_standard_storage, node_copy_standard_storage);
  /* remove Vector input for Nishita */
  node_type_update(&ntype, node_shader_update_sky_spectral);

  nodeRegisterType(&ntype);
}
