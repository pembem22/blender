#!/usr/bin/env python
# -*- coding: utf-8 -*-

import PyOpenColorIO
import numpy
import colour
from colour import (
    io,
    adaptation,
    models
)


def as_numeric(obj, as_type=numpy.float64):
    try:
        return as_type(obj)
    except TypeError:
        return obj


def shape_OCIO_matrix(numpy_matrix):
    # Shape the RGB to XYZ array for OpenColorIO
    ocio_matrix = numpy.pad(
        numpy_matrix,
        [(0, 1), (0, 1)],
        mode='constant'
    )
    ocio_matrix = ocio_matrix.flatten()
    ocio_matrix[-1] = 1.

    return ocio_matrix


# Convert relative EV to radiometric linear value.
def calculate_ev_to_rl(
    in_ev,
    rl_middle_grey=0.18
):
    in_ev = numpy.asarray(in_ev)

    return as_numeric(numpy.power(2., in_ev) * rl_middle_grey)


# Convert radiometric linear value to relative EV
def calculate_rl_to_ev(
    in_rl,
    rl_middle_grey=0.18
):
    in_rl = numpy.asarray(in_rl)

    return as_numeric(numpy.log2(in_rl) - numpy.log2(rl_middle_grey))


if __name__ == "__main__":
    output_config_name = "config.ocio"
    path_prefix_luts = "./LUTs/"
    LUT_search_paths = ["LUTs"]

    in_xy_D65 = models.sRGB_COLOURSPACE.whitepoint

    sRGB_IE = colour.RGB_Colourspace(
        "sRGB IE",
        models.sRGB_COLOURSPACE.primaries,
        [1./3., 1./3.],
        whitepoint_name="Illuminant E",
        cctf_decoding=models.sRGB_COLOURSPACE.cctf_decoding,
        cctf_encoding=models.sRGB_COLOURSPACE.cctf_encoding,
        use_derived_RGB_to_XYZ_matrix=True,
        use_derived_XYZ_to_RGB_matrix=True)

    sRGB_D65 = colour.RGB_Colourspace(
        "sRGB",
        models.sRGB_COLOURSPACE.primaries,
        models.sRGB_COLOURSPACE.whitepoint,
        models.sRGB_COLOURSPACE.whitepoint_name,
        cctf_decoding=models.sRGB_COLOURSPACE.cctf_decoding,
        cctf_encoding=models.sRGB_COLOURSPACE.cctf_encoding,
        use_derived_RGB_to_XYZ_matrix=True,
        use_derived_XYZ_to_RGB_matrix=True)

    sRGB_domain = numpy.array([0.0, 1.0])
    sRGB_tf_to_linear_LUT = colour.LUT1D(
        table=models.sRGB_COLOURSPACE.cctf_decoding(
            colour.LUT1D.linear_table(
                1024, sRGB_domain)),
        name="sRGB to Linear",
        domain=sRGB_domain,
        comments=["sRGB CCTF to Display Linear"])
    sRGB_linear_to_tf_LUT = colour.LUT1D(
        table=models.sRGB_COLOURSPACE.cctf_encoding(
            colour.LUT1D.linear_table(
                8192, sRGB_domain)),
        name="Linear to sRGB",
        domain=sRGB_domain,
        comments=["sRGB Display Linear to CCTF"])

    sRGB_D65_to_IE_RGB_to_RGB_matrix = colour.RGB_to_RGB_matrix(
        sRGB_D65,
        sRGB_IE)

    io.write_LUT(
        LUT=sRGB_tf_to_linear_LUT,
        path=path_prefix_luts + "sRGB_CCTF_to_Linear.spi1d",
        decimals=10)
    io.write_LUT(
        LUT=sRGB_linear_to_tf_LUT,
        path=path_prefix_luts + "sRGB_Linear_to_CCTF.spi1d",
        decimals=10)

    config = PyOpenColorIO.Config()

    config.setSearchPath(":".join(LUT_search_paths))

    # Define the radiometrically linear RGB space with BT.709 primaries and
    # assumed adapted Illuminant E based white point.
    colourspace = PyOpenColorIO.ColorSpace(
        family="Colourspace",
        name="Linear")
    colourspace.setDescription(
        "Linear BT.709 based reference with illuminant E based white point")
    colourspace.setBitDepth(PyOpenColorIO.Constants.BIT_DEPTH_F32)
    colourspace.setAllocationVars(
        [
            numpy.log2(calculate_ev_to_rl(-10.0)),
            numpy.log2(calculate_ev_to_rl(15.0))
        ])
    colourspace.setAllocation(PyOpenColorIO.Constants.ALLOCATION_LG2)
    config.addColorSpace(colourspace)

    # Define the sRGB specification
    colourspace = PyOpenColorIO.ColorSpace(
        family="Colourspace",
        name="sRGB Colourspace")
    colourspace.setDescription("sRGB IEC 61966-2-1 Colourspace")
    colourspace.setBitDepth(PyOpenColorIO.Constants.BIT_DEPTH_F32)
    colourspace.setAllocationVars([0.0, 1.0])
    colourspace.setAllocation(PyOpenColorIO.Constants.ALLOCATION_UNIFORM)
    transform_to = PyOpenColorIO.GroupTransform()
    transform_to.push_back(
        PyOpenColorIO.FileTransform(
            "sRGB_CCTF_to_Linear.spi1d",
            interpolation=PyOpenColorIO.Constants.INTERP_NEAREST))

    ocio_sRGB_D65_to_sRGB_IE_matrix = shape_OCIO_matrix(
        sRGB_D65_to_IE_RGB_to_RGB_matrix)
    transform_sRGB_D65_to_sRGB_IE =\
        PyOpenColorIO.MatrixTransform(ocio_sRGB_D65_to_sRGB_IE_matrix)
    transform_to.push_back(transform_sRGB_D65_to_sRGB_IE)
    transform_from = PyOpenColorIO.GroupTransform()

    # The first object is the wrong direction re-used from above.
    transform_sRGB_IE_to_sRGB_D65 =\
        PyOpenColorIO.MatrixTransform(ocio_sRGB_D65_to_sRGB_IE_matrix)

    # Flip the direction to get the proper output
    transform_sRGB_IE_to_sRGB_D65.setDirection(
        PyOpenColorIO.Constants.TRANSFORM_DIR_INVERSE)

    transform_from.push_back(transform_sRGB_IE_to_sRGB_D65)
    transform_from.push_back(
        PyOpenColorIO.FileTransform(
            "sRGB_Linear_to_CCTF.spi1d",
            interpolation=PyOpenColorIO.Constants.INTERP_NEAREST))

    colourspace.setTransform(
        transform_to, PyOpenColorIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
    colourspace.setTransform(
        transform_from, PyOpenColorIO.Constants.COLORSPACE_DIR_FROM_REFERENCE)

    config.addColorSpace(colourspace)

    # Define the commodity sRGB transform
    colourspace = PyOpenColorIO.ColorSpace(
        family="Colourspace",
        name="BT.709 2.2 CCTF Colourspace")
    colourspace.setDescription("Commodity BT.709 2.2 CCTF Colourspace")
    colourspace.setBitDepth(PyOpenColorIO.Constants.BIT_DEPTH_F32)
    colourspace.setAllocationVars([0.0, 1.0])
    colourspace.setAllocation(PyOpenColorIO.Constants.ALLOCATION_UNIFORM)
    transform_to = PyOpenColorIO.GroupTransform()
    transform_to.push_back(
        PyOpenColorIO.ExponentTransform([2.2, 2.2, 2.2, 1.0]))

    transform_to.push_back(transform_sRGB_D65_to_sRGB_IE)
    transform_from = PyOpenColorIO.GroupTransform()

    transform_from.push_back(transform_sRGB_IE_to_sRGB_D65)
    exponent_transform = PyOpenColorIO.ExponentTransform([2.2, 2.2, 2.2, 1.0])
    exponent_transform.setDirection(
        PyOpenColorIO.Constants.TRANSFORM_DIR_INVERSE)
    transform_from.push_back(exponent_transform)

    colourspace.setTransform(
        transform_to, PyOpenColorIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
    colourspace.setTransform(
        transform_from, PyOpenColorIO.Constants.COLORSPACE_DIR_FROM_REFERENCE)

    config.addColorSpace(colourspace)

    # Define the reference ITU-R BT.709 linear RGB
    colourspace = PyOpenColorIO.ColorSpace(
        family="Chromaticity",
        name="sRGB Linear")
    colourspace.setDescription(
        "sRGB IEC 61966-2-1 primaries with linear transfer function")
    colourspace.setBitDepth(PyOpenColorIO.Constants.BIT_DEPTH_F32)
    colourspace.setAllocationVars(
        [
            numpy.log2(calculate_ev_to_rl(-10.0)),
            numpy.log2(calculate_ev_to_rl(15.0))
        ])
    colourspace.setAllocation(PyOpenColorIO.Constants.ALLOCATION_LG2)

    transform_to = transform_sRGB_D65_to_sRGB_IE
    transform_from = transform_sRGB_IE_to_sRGB_D65

    colourspace.setTransform(
        transform_to, PyOpenColorIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
    colourspace.setTransform(
        transform_from, PyOpenColorIO.Constants.COLORSPACE_DIR_FROM_REFERENCE)

    config.addColorSpace(colourspace)

    # Define the XYZ space relative to this reference
    colourspace = PyOpenColorIO.ColorSpace(
        family="Chromaticity",
        name="Linear XYZ IE")
    colourspace.setDescription(
        "XYZ transform with assumed IE white point")
    colourspace.setBitDepth(PyOpenColorIO.Constants.BIT_DEPTH_F32)
    colourspace.setAllocationVars(
        [
            numpy.log2(calculate_ev_to_rl(-10.0)),
            numpy.log2(calculate_ev_to_rl(15.0))
        ])
    colourspace.setAllocation(PyOpenColorIO.Constants.ALLOCATION_LG2)

    # Make the original transform
    ocioshape_sRGB_IE_to_XYZ = shape_OCIO_matrix(sRGB_IE.RGB_to_XYZ_matrix)
    ociotransform_sRGB_IE_to_XYZ =\
        PyOpenColorIO.MatrixTransform(ocioshape_sRGB_IE_to_XYZ)

    # Make a copy, with wrong direction, then set direction
    ociotransform_XYZ_to_sRGB_IE =\
        PyOpenColorIO.MatrixTransform(ocioshape_sRGB_IE_to_XYZ)
    ociotransform_XYZ_to_sRGB_IE.setDirection(
        PyOpenColorIO.Constants.TRANSFORM_DIR_INVERSE)

    transform_to = ociotransform_XYZ_to_sRGB_IE
    transform_from = ociotransform_sRGB_IE_to_XYZ

    colourspace.setTransform(
        transform_to, PyOpenColorIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
    colourspace.setTransform(
        transform_from, PyOpenColorIO.Constants.COLORSPACE_DIR_FROM_REFERENCE)

    config.addColorSpace(colourspace)

    # Define the Non-Colour Data transform
    colorspace = PyOpenColorIO.ColorSpace(
        family="Data",
        name="Float Data")
    colorspace.setDescription("Float data that does not define a colour")
    colorspace.setBitDepth(PyOpenColorIO.Constants.BIT_DEPTH_F32)
    colorspace.setIsData(True)
    config.addColorSpace(colorspace)

    # Define the luminance coefficients for the configuration
    config.setDefaultLumaCoefs(sRGB_IE.RGB_to_XYZ_matrix[1])

    config.setRole(
        PyOpenColorIO.Constants.ROLE_SCENE_LINEAR,
        "Linear")
    config.setRole(
        PyOpenColorIO.Constants.ROLE_REFERENCE,
        "Linear")
    config.setRole(
        PyOpenColorIO.Constants.ROLE_COLOR_TIMING,
        "sRGB Colourspace")
    config.setRole(
        PyOpenColorIO.Constants.ROLE_COMPOSITING_LOG,
        "sRGB Colourspace")
    config.setRole(
        PyOpenColorIO.Constants.ROLE_COLOR_PICKING,
        "sRGB Colourspace")
    config.setRole(
        PyOpenColorIO.Constants.ROLE_DATA,
        "Float Data")
    config.setRole(
        PyOpenColorIO.Constants.ROLE_DEFAULT,
        "sRGB Colourspace")
    config.setRole(
        PyOpenColorIO.Constants.ROLE_MATTE_PAINT,
        "sRGB Colourspace")
    config.setRole(
        PyOpenColorIO.Constants.ROLE_TEXTURE_PAINT,
        "sRGB Colourspace")

    # Define the Blender Role RGB to XYZ
    config.setRole(
        "XYZ",
        "Linear XYZ IE")

    displays = {
        "sRGB-Like Commodity Display": {
            "Display Native": "BT.709 2.2 CCTF Colourspace"
        },
        "sRGB Display": {
            "Display Native": "sRGB Colourspace"
        }
    }

    all_displays = set()
    all_views = set()

    for display, views in displays.items():
        all_displays.add(display)
        for view, transform in views.items():
            all_views.add(view)
            config.addDisplay(display, view, transform)

    config.setActiveDisplays(", ".join(all_displays))
    config.setActiveViews(", ".join(all_views))

    try:
        config.sanityCheck()
        write_file = open(output_config_name, "w")
        write_file.write(config.serialize())
        write_file.close()
        print("Wrote config \"{}\"".format(output_config_name))
    except Exception as ex:
        raise ex
