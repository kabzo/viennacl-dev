/* =========================================================================
   Copyright (c) 2010-2014, Institute for Microelectronics,
                            Institute for Analysis and Scientific Computing,
                            TU Wien.
   Portions of this software are copyright by UChicago Argonne, LLC.

                            -----------------
                  ViennaCL - The Vienna Computing Library
                            -----------------

   Project Head:    Karl Rupp                   rupp@iue.tuwien.ac.at

   (A list of authors and contributors can be found in the PDF manual)

   License:         MIT (X11), see file LICENSE in the base directory
============================================================================= */

/*
Solutions for testdata were generated with Scilab line:

M=fscanfMat('nsm1.example');e=spec(M);e=gsort(e);rr=real(e);ii=imag(e);e=cat(1, rr, ii); s=strcat(string(e), ' ');write('tmp', s);
*/

#ifndef NDEBUG
  #define NDEBUG
#endif

//#define VIENNACL_DEBUG_ALL
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>

#include "viennacl/linalg/prod.hpp"
#include "viennacl/linalg/qr-method.hpp"

#include <examples/benchmarks/benchmark-utils.hpp>

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>

namespace ublas = boost::numeric::ublas;

typedef float ScalarType;

const ScalarType EPS = 0.0001f;

void read_matrix_size(std::fstream& f, std::size_t& sz)
{
    if(!f.is_open())
    {
        throw std::invalid_argument("File is not opened");
    }

    f >> sz;
}

template <typename MatrixLayout>
void read_matrix_body(std::fstream& f, viennacl::matrix<ScalarType, MatrixLayout>& A)
{
    if(!f.is_open())
    {
        throw std::invalid_argument("File is not opened");
    }

    boost::numeric::ublas::matrix<ScalarType> h_A(A.size1(), A.size2());

    for(std::size_t i = 0; i < h_A.size1(); i++) {
        for(std::size_t j = 0; j < h_A.size2(); j++) {
            ScalarType val = 0.0;
            f >> val;
            h_A(i, j) = val;
        }
    }

    viennacl::copy(h_A, A);
}

void read_vector_body(std::fstream& f, std::vector<ScalarType>& v) {
    if(!f.is_open())
        throw std::invalid_argument("File is not opened");

    for(std::size_t i = 0; i < v.size(); i++)
    {
            ScalarType val = 0.0;
            f >> val;
            v[i] = val;
    }
}

template <typename MatrixLayout>
bool check_tridiag(viennacl::matrix<ScalarType, MatrixLayout>& A_orig)
{
    ublas::matrix<ScalarType> A(A_orig.size1(), A_orig.size2());
    viennacl::copy(A_orig, A);

    for (unsigned int i = 0; i < A.size1(); i++) {
        for (unsigned int j = 0; j < A.size2(); j++) {
            if ((std::abs(A(i, j)) > EPS) && ((i - 1) != j) && (i != j) && ((i + 1) != j))
            {
                // std::cout << "Failed at " << i << " " << j << " " << A(i, j) << "\n";
                return false;
            }
        }
    }
    return true;
}

template <typename MatrixLayout>
bool check_hessenberg(viennacl::matrix<ScalarType, MatrixLayout>& A_orig)
{
    ublas::matrix<ScalarType> A(A_orig.size1(), A_orig.size2());
    viennacl::copy(A_orig, A);

    for (std::size_t i = 0; i < A.size1(); i++) {
        for (std::size_t j = 0; j < A.size2(); j++) {
            if ((std::abs(A(i, j)) > EPS) && (i > (j + 1)))
            {
                // std::cout << "Failed at " << i << " " << j << " " << A(i, j) << "\n";
                return false;
            }
        }
    }
    return true;
}

ScalarType matrix_compare(ublas::matrix<ScalarType>& res,
                            ublas::matrix<ScalarType>& ref)
{
    ScalarType diff = 0.0;
    ScalarType mx = 0.0;

    for(std::size_t i = 0; i < res.size1(); i++)
    {
        for(std::size_t j = 0; j < res.size2(); j++)
        {
            diff = std::max(diff, std::abs(res(i, j) - ref(i, j)));
            mx = std::max(mx, res(i, j));
        }
    }

    return diff / mx;
}

ScalarType vector_compare(std::vector<ScalarType> & res,
                          std::vector<ScalarType> & ref)
{
    std::sort(ref.begin(), ref.end());
    std::sort(res.begin(), res.end());

    ScalarType diff = 0.0;
    ScalarType mx = 0.0;
    for(size_t i = 0; i < res.size(); i++)
    {
        diff = std::max(diff, std::abs(res[i] - ref[i]));
        mx = std::max(mx, res[i]);
    }

    return diff / mx;
}

template <typename MatrixLayout>
void matrix_print(viennacl::matrix<ScalarType, MatrixLayout>& A)
{
    for (unsigned int i = 0; i < A.size1(); i++) {
        for (unsigned int j = 0; j < A.size2(); j++)
           std::cout << std::fixed << A(i, j) << "\t";
        std::cout << "\n";
    }
}

template <typename MatrixLayout>
void test_eigen(const std::string& fn, bool is_symm)
{
    std::cout << "Reading..." << "\n";
    std::size_t sz;
    // read file
    std::fstream f(fn.c_str(), std::fstream::in);
    //read size of input matrix
    read_matrix_size(f, sz);

    if (viennacl::is_row_major<MatrixLayout>::value)
      std::cout << "Testing row-major matrix of size " << sz << "-by-" << sz << std::endl;
    else
      std::cout << "Testing column-major matrix of size " << sz << "-by-" << sz << std::endl;

    viennacl::matrix<ScalarType> A_input(sz, sz), A_ref(sz, sz), Q(sz, sz);
    // reference vector with reference values from file
    std::vector<ScalarType> eigen_ref_re(sz);
    // calculated real eigenvalues
    std::vector<ScalarType> eigen_re(sz);
    // calculated im. eigenvalues
    std::vector<ScalarType> eigen_im(sz);

    // read input matrix from file
    read_matrix_body(f, A_input);
    // read reference eigenvalues from file
    read_vector_body(f, eigen_ref_re);


    f.close();

    A_ref = A_input;

    std::cout << "Calculation..." << "\n";

    Timer timer;
    timer.start();
    // Start the calculation
    if(is_symm)
        viennacl::linalg::qr_method_sym(A_input, Q, eigen_re);
    else
        viennacl::linalg::qr_method_nsm(A_input, Q, eigen_re, eigen_im);
/*

    std::cout << "\n\n Matrix A: \n\n";
    matrix_print(A_input);
    std::cout << "\n\n";

    std::cout << "\n\n Matrix Q: \n\n";
    matrix_print(Q);
    std::cout << "\n\n";
*/

    double time_spend = timer.get();

    std::cout << "Verification..." << "\n";

    bool is_hessenberg = check_hessenberg(A_input);
    bool is_tridiag = check_tridiag(A_input);

    ublas::matrix<ScalarType> A_ref_ublas(sz, sz), A_input_ublas(sz, sz), Q_ublas(sz, sz), result1(sz, sz), result2(sz, sz);
    viennacl::copy(A_ref, A_ref_ublas);
    viennacl::copy(A_input, A_input_ublas);
    viennacl::copy(Q, Q_ublas);

    // compute result1 = ublas::prod(Q_ublas, A_input_ublas);   (terribly slow when using ublas directly)
    for (std::size_t i=0; i<result1.size1(); ++i)
      for (std::size_t j=0; j<result1.size2(); ++j)
      {
        ScalarType value = 0;
        for (std::size_t k=0; k<Q_ublas.size2(); ++k)
          value += Q_ublas(i, k) * A_input_ublas(k, j);
        result1(i,j) = value;
      }
    // compute result2 = ublas::prod(A_ref_ublas, Q_ublas);   (terribly slow when using ublas directly)
    for (std::size_t i=0; i<result2.size1(); ++i)
      for (std::size_t j=0; j<result2.size2(); ++j)
      {
        ScalarType value = 0;
        for (std::size_t k=0; k<A_ref_ublas.size2(); ++k)
          value += A_ref_ublas(i, k) * Q_ublas(k, j);
        result2(i,j) = value;
      }


    ScalarType prods_diff = matrix_compare(result1, result2);
    ScalarType eigen_diff = vector_compare(eigen_re, eigen_ref_re);


    bool is_ok = is_hessenberg;

    if(is_symm)
        is_ok = is_ok && is_tridiag;

    is_ok = is_ok && (eigen_diff < EPS);
    is_ok = is_ok && (prods_diff < EPS);

    // std::cout << A_ref << "\n";
    // std::cout << A_input << "\n";
    // std::cout << Q << "\n";
    // std::cout << eigen_re << "\n";
    // std::cout << eigen_im << "\n";
    // std::cout << eigen_ref_re << "\n";
    // std::cout << eigen_ref_im << "\n";

    // std::cout << result1 << "\n";
    // std::cout << result2 << "\n";
    // std::cout << eigen_ref << "\n";
    // std::cout << eigen << "\n";

    printf("%6s [%dx%d] %40s time = %.4f\n", is_ok?"[[OK]]":"[FAIL]", (int)A_ref.size1(), (int)A_ref.size2(), fn.c_str(), time_spend);
    printf("tridiagonal = %d, hessenberg = %d prod-diff = %f eigen-diff = %f\n", is_tridiag, is_hessenberg, prods_diff, eigen_diff);
    std::cout << std::endl << std::endl;

    if (!is_ok)
      exit(EXIT_FAILURE);

}

int main()
{

  test_eigen<viennacl::row_major>("../examples/testdata/eigen/symm5.example", true);
 // test_eigen<viennacl::row_major>("../../examples/testdata/eigen/symm3.example", true);  // Computation of this matrix takes very long

  test_eigen<viennacl::column_major>("../examples/testdata/eigen/symm5.example", true);
//  test_eigen<viennacl::column_major>("../../examples/testdata/eigen/symm3.example", true);

#ifdef VIENNACL_WITH_OPENCL
  test_eigen<viennacl::row_major>("../examples/testdata/eigen/nsm2.example", false);
#endif
  //test_eigen<viennacl::row_major>("../../examples/testdata/eigen/nsm2.example", false);
  //test_eigen("../../examples/testdata/eigen/nsm3.example", false);
  //test_eigen("../../examples/testdata/eigen/nsm4.example", false); //Note: This test suffers from round-off errors in single precision, hence disabled

  std::cout << std::endl;
  std::cout << "------- Test completed --------" << std::endl;
  std::cout << std::endl;

  return EXIT_SUCCESS;
}
