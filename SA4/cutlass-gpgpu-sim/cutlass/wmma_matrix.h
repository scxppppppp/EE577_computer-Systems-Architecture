/***************************************************************************************************
 * Copyright (c) 2017-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of
 *       conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *     * Neither the name of the NVIDIA CORPORATION nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL NVIDIA CORPORATION BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TOR (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **************************************************************************************************/
/*! \file
    \brief Abstractions for loading and storing matrices using the CUDA WMMA API.
*/
#pragma once

#if defined(__CUDACC__) && (!defined(__CUDA_ARCH__) || __CUDA_ARCH__ >= 700)

// Dependent header files should use the following macro to guard all code using
// nvcuda::wmma:: to enable compilation for CUDA Compute Capabilities < sm_70.
// Earlier shader models not support Tensor Cores.
#define CUTLASS_USE_WMMA_API

#include "stdio.h"

//#include <crt/mma.h>
#include <mma.h>
#include <cutlass/fragment.h>
#include <cutlass/load_store.h>
#include <cutlass/matrix_traits.h>
#include <cutlass/shape.h>
#include <cutlass/vector.h>

namespace cutlass {

////////////////////////////////////////////////////////////////////////////////////////////////////

/// Statically maps cutlass::MatrixLayout => nvcuda::wmma layout tags
template <MatrixLayout::Kind kLayout_>
struct WmmaLayout {
  typedef nvcuda::wmma::col_major Layout;
};

/// Statically maps cutlass::MatrixLayout => nvcuda::wmma layout tags
template <>
struct WmmaLayout<MatrixLayout::kRowMajor> {
  typedef nvcuda::wmma::row_major Layout;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

/// Adapter to nvcuda::wmma fragment load and store operations
template <GemmOperand::Kind kOperand_,
          MatrixLayout::Kind kLayout_,
          typename Scalar_,
          typename WmmaShape_>
struct WmmaMatrix {};

////////////////////////////////////////////////////////////////////////////////////////////////////

/// Adapter to nvcuda::wmma fragment accessors for A operand
template <MatrixLayout::Kind kLayout_, typename Scalar_, typename WmmaShape_>
struct WmmaMatrix<GemmOperand::kA, kLayout_, Scalar_, WmmaShape_>
    : public nvcuda::wmma::fragment<
          /// The nvcuda::wmma operand name.
          nvcuda::wmma::matrix_a,
          /// The dimensions.
          WmmaShape_::kW,
          WmmaShape_::kH,
          WmmaShape_::kD,
          /// The scalar.
          Scalar_,
          /// The layout.
          typename WmmaLayout<kLayout_>::Layout> {
  /// This type.
  typedef WmmaMatrix<GemmOperand::kA, kLayout_, Scalar_, WmmaShape_> This_;

  /// Fill-in the element.
  CUTLASS_DEVICE This_& operator=(Scalar_ const& x) {
    nvcuda::wmma::fill_fragment(*this, x);
    return *this;
  }

  /// Load from memory.
  CUTLASS_DEVICE void load(Scalar_ const* pointer, int const stride) {
    nvcuda::wmma::load_matrix_sync(*this, pointer, stride);
  }

  /// Store to memory.
  CUTLASS_DEVICE void store(Scalar_* pointer, int const stride) const {
    nvcuda::wmma::store_matrix_sync(pointer, *this, stride);
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////

/// Adapter to nvcuda::wmma fragment accessors for B operand
template <MatrixLayout::Kind kLayout_, typename Scalar_, typename WmmaShape_>
struct WmmaMatrix<GemmOperand::kB, kLayout_, Scalar_, WmmaShape_>
    : public nvcuda::wmma::fragment<
          /// The nvcuda::wmma operand name.
          nvcuda::wmma::matrix_b,
          /// The dimensions.
          WmmaShape_::kW,
          WmmaShape_::kH,
          WmmaShape_::kD,
          /// The scalar.
          Scalar_,
          /// The layout.
          typename WmmaLayout<kLayout_>::Layout> {
  /// This type.
  typedef WmmaMatrix<GemmOperand::kB, kLayout_, Scalar_, WmmaShape_> This_;

  /// Fill-in the element.
  CUTLASS_DEVICE This_& operator=(Scalar_ const& x) {
    nvcuda::wmma::fill_fragment(*this, x);
    return *this;
  }

  /// Load from memory.
  CUTLASS_DEVICE void load(Scalar_ const* pointer, int const stride) {
    nvcuda::wmma::load_matrix_sync(*this, pointer, stride);
  }

  /// Store to memory.
  CUTLASS_DEVICE void store(Scalar_* pointer, int const stride) const {
    nvcuda::wmma::store_matrix_sync(pointer, *this, stride);
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////

/// Adapter to nvcuda::wmma fragment accessors for C operand
template <MatrixLayout::Kind kLayout_, typename Scalar_, typename WmmaShape_>
struct WmmaMatrix<GemmOperand::kC, kLayout_, Scalar_, WmmaShape_>
    : public nvcuda::wmma::fragment<
          /// The nvcuda::wmma operand name.
          nvcuda::wmma::accumulator,
          /// The dimensions.
          WmmaShape_::kW,
          WmmaShape_::kH,
          WmmaShape_::kD,
          /// The scalar.
          Scalar_> {
  /// This type.
  typedef WmmaMatrix<GemmOperand::kC, kLayout_, Scalar_, WmmaShape_> This_;
  /// The layout.
  static MatrixLayout::Kind const kLayout = kLayout_;

  /// Fill-in the element.
  CUTLASS_DEVICE This_& operator=(Scalar_ const& x) {
    nvcuda::wmma::fill_fragment(*this, x);
    return *this;
  }

  /// Load from memory.
  CUTLASS_DEVICE void load(Scalar_ const* pointer, int const stride) {
    bool const kIsRowMajor = kLayout == MatrixLayout::kRowMajor;
    nvcuda::wmma::load_matrix_sync(
        *this,
        pointer,
        stride,
        kIsRowMajor ? nvcuda::wmma::mem_row_major : nvcuda::wmma::mem_col_major);
  }

  /// Store to memory.
  CUTLASS_DEVICE void store(Scalar_* pointer, int const stride) const {
    bool const kIsRowMajor = kLayout == MatrixLayout::kRowMajor;
    nvcuda::wmma::store_matrix_sync(
        pointer,
        *this,
        stride,
        kIsRowMajor ? nvcuda::wmma::mem_row_major : nvcuda::wmma::mem_col_major);
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}  // namespace cutlass

#endif  // defined CUTLASS_USE_WMMA_API
