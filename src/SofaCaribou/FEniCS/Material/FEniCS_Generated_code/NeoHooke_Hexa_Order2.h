// This code conforms with the UFC specification version 2018.2.0.dev0
// and was automatically generated by FFCx version 0.3.1.dev0.
//
// This code was generated with the following parameters:
//
//  {'assume_aligned': -1,
//   'epsilon': 1e-14,
//   'output_directory': '../../FEniCS_Generated_code/',
//   'padlen': 1,
//   'profile': False,
//   'scalar_type': 'double',
//   'table_atol': 1e-09,
//   'table_rtol': 1e-06,
//   'tabulate_tensor_void': False,
//   'ufl_file': ['NeoHooke_Hexa_Order2.py'],
//   'verbosity': 30,
//   'visualise': False}


#pragma once

#include <ufcx.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ufcx_finite_element element_c8529aa6bb4ec86f198ffb51079cb7bdc26ab695;

extern ufcx_finite_element element_78155b1a0b0cce247b83d6736bd1f1f9f86f33f2;

extern ufcx_finite_element element_e89091147189a7222c0ac916eaa9284aa103e2e3;

extern ufcx_finite_element element_0decba65b5760cccdf759ddb54557092ccf8ebde;

extern ufcx_dofmap dofmap_c8529aa6bb4ec86f198ffb51079cb7bdc26ab695;

extern ufcx_dofmap dofmap_78155b1a0b0cce247b83d6736bd1f1f9f86f33f2;

extern ufcx_dofmap dofmap_e89091147189a7222c0ac916eaa9284aa103e2e3;

extern ufcx_dofmap dofmap_0decba65b5760cccdf759ddb54557092ccf8ebde;

extern ufcx_integral integral_131e4e64ba94c853f88087e0a6543d0e89eeafde;

extern ufcx_integral integral_8518c14de8d8d8a496830d26ee966731a24b801e;

extern ufcx_form form_bae5f1ed6f5568b521f57bbe9f17050c50a9cec5;

// Helper used to create form using name which was given to the
// form in the UFL file.
// This helper is called in user c++ code.
//
extern ufcx_form* form_NeoHooke_Hexa_Order2_F;

// Helper used to create function space using function name
// i.e. name of the Python variable.
//
ufcx_function_space* functionspace_form_NeoHooke_Hexa_Order2_F(const char* function_name);

extern ufcx_form form_99191367d1d900ce5296e9451225889cb2afc477;

// Helper used to create form using name which was given to the
// form in the UFL file.
// This helper is called in user c++ code.
//
extern ufcx_form* form_NeoHooke_Hexa_Order2_J;

// Helper used to create function space using function name
// i.e. name of the Python variable.
//
ufcx_function_space* functionspace_form_NeoHooke_Hexa_Order2_J(const char* function_name);

#ifdef __cplusplus
}
#endif