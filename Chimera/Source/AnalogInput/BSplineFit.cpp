#include "stdafx.h"
#include "BSplineFit.h"
#include <gsl/gsl_cdf.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_poly.h>
#include <algorithm>
#include <qdebug.h>


BSplineFit::BSplineFit(const BSplineFit& bsfit)
{
    emptyStart = bsfit.emptyStart;
    if (emptyStart) {
        return;
    }
    else {
        initialize(bsfit.dataSize, bsfit.datax, bsfit.datay, bsfit.orderBSpline, bsfit.nBreak);
        solve_system();
    }
    
}

BSplineFit& BSplineFit::operator=(const BSplineFit& bsfit)
{
    emptyStart = bsfit.emptyStart;
    if (!emptyStart) {
        initialize(bsfit.dataSize, bsfit.datax, bsfit.datay, bsfit.orderBSpline, bsfit.nBreak);
        solve_system();
    }
    return *this;
}

BSplineFit::BSplineFit() : emptyStart(true)
{
}

BSplineFit::~BSplineFit()
{
    if (!emptyStart) {
        freeAll();
    }
    
}

void BSplineFit::initialize(unsigned _dataSize, std::vector<double> _datax, std::vector<double> _datay,
    unsigned _orderBSpline, unsigned _nBreak)
{
    if (!isMonotonic(_datax)) {
        thrower("The BSplineFit _datax is not monotonic. \r\n\t That is probably because the calibration result is used as x-value in the fitting "
            "and this result can be corrupted (most cases are the result is all close to zero) .And for BSpline, the x-value need to be monotonic.");
    }

    if (_dataSize != _datax.size() || _dataSize != _datay.size()) {
        thrower("Error in Spline fitting: the input data size is not equal to the expected one");
    }
    emptyStart = false;

    dataSize = _dataSize;
    datax = _datax;
    datay = _datay;
    orderBSpline = _orderBSpline;;
    nBreak = _nBreak;
    nBasis = nBreak - 2 + orderBSpline;
    TSS = -1; RSS = -1; Rsq = -1;

    bw = gsl_bspline_alloc(orderBSpline, nBreak);
    mw = gsl_multifit_linear_alloc(dataSize, nBasis);
    X = gsl_matrix_alloc(dataSize, nBasis);
    gsl_bspline_knots_uniform(datax[0], datax[dataSize - 1], bw);

    /*assign datay to member gsl_vector*/
    y = gsl_vector_alloc(dataSize);
    for (unsigned i = 0; i < dataSize; i++)
    {
        gsl_vector_set(y, i, datay[i]);
    }
    /*allocate coefficent vector and covariance matrix*/
    coef = gsl_vector_alloc(nBasis);
    cov = gsl_matrix_alloc(nBasis, nBasis);

    /* construct the fit matrix X */
    B = gsl_vector_alloc(nBasis);
    for (unsigned i = 0; i < dataSize; ++i)
    {
        /* compute B_j(xi) for all j */
        gsl_bspline_eval(datax[i], B, bw);
        /* fill in row i of X */
        for (unsigned j = 0; j < nBasis; ++j)
        {
            double Bj = gsl_vector_get(B, j);
            gsl_matrix_set(X, i, j, Bj);
        }
    }
}

void BSplineFit::freeAll()
{
    gsl_matrix_free(X);
    gsl_vector_free(y);
    gsl_vector_free(coef);
    gsl_matrix_free(cov);
    gsl_vector_free(B);

    gsl_bspline_free(bw);
    gsl_multifit_linear_free(mw);
}


/* do the fit */
void BSplineFit::solve_system()
{
    gsl_multifit_linear(X, y, coef, cov, &RSS, mw);
    TSS = gsl_stats_tss(y->data, y->stride, y->size);
    Rsq = 1.0 - RSS / TSS;

    confi95 = getConfidentInterval(0.95);
    leftPoly = getBoudaryPolyCoef(getxlim().first);
    rightPoly = getBoudaryPolyCoef(getxlim().second);
}

double BSplineFit::calculateY(double x)
{
    if (emptyStart) {
        return 0;
    }
    double yi, yerr;
    if (x <= getxlim().first ) {
        yi = gsl_poly_eval(leftPoly.data(), leftPoly.size(), x);
    }
    else if (x >= getxlim().second) {
        yi = gsl_poly_eval(rightPoly.data(), rightPoly.size(), x);
    }
    else {
        gsl_bspline_eval(x, B, bw); // B is assigned values of BSplines at x points of all basis functions
        // yerr is calculated through B^T * cov * B from directly apply covaiance operator to Y, see my oneNote
        gsl_multifit_linear_est(B, coef, cov, &yi, &yerr);
    }
    return yi;
    
}

std::vector<double> BSplineFit::calculateY(std::vector<double> x)
{
    std::vector<double> y;
    for (unsigned i = 0; i < x.size(); i++)
    {
        y.push_back(calculateY(x[i]));
    }
    return y;
}

std::vector<double> BSplineFit::getConfidentInterval(double level)
{
    auto covDiagView = gsl_matrix_diagonal(cov);
    gsl_vector& covDiag = covDiagView.vector;
    double coefxx = gsl_cdf_tdist_Qinv((1 - level) / 2, dataSize - nBasis /*dof*/);
    auto confixx = std::vector<double>(covDiag.data, covDiag.data + covDiag.size);
    std::for_each(confixx.begin(), confixx.end(), [coefxx](auto& confid) {
        confid = coefxx * std::sqrt(confid); });
    return confixx;
}

std::vector<double> BSplineFit::getBoudaryPolyCoef(double boundaryPos)
{
    gsl_matrix* dB = gsl_matrix_alloc(nBasis, orderBSpline); 
    gsl_vector* tmp = gsl_vector_alloc(orderBSpline);
    gsl_bspline_deriv_eval(boundaryPos, orderBSpline - 1, dB, bw); // orderBSpline - 1 is the highest order of derivative
    gsl_blas_dgemv(CblasTrans, 1.0, dB, coef, 0.0, tmp); // tmp stores Transpose(dB)*coef
    // calculate the derivative matrix of the polynomial 
    // use c[0] + c[1]*x + ... for various c[] (i.e c={1,0,0,...}, c={0,1,0,0,...}, c={0,0,1,0,0,...}) 
    // to generate derivate matrix {{1,x,x^2,...,x^n},
                                //  {0,1,2*x,...,n*x^(n-1)},
                                //  {0,0,2,...,n*(n-1)*x^(n-2)}, ...}
    gsl_matrix* derMat = gsl_matrix_alloc(orderBSpline, orderBSpline); 
    for (size_t i = 0; i < orderBSpline; i++)
    {
        std::vector<double> coefPoly(orderBSpline, 0.0);
        std::vector<double> der(orderBSpline, 0.0);
        coefPoly[i] = 1.0; // set i-th element to be 1 and get i-th coloum of derivative matrix
        gsl_poly_eval_derivs(coefPoly.data(), coefPoly.size(), boundaryPos, der.data(), der.size());
        for (size_t j = 0; j < orderBSpline; j++)
        {
            gsl_matrix_set(derMat, j, i, der[j]);
        }
    }
    // calculate inverse of the derivative matrix, which is to be multiplied to tmp = Transpose(dB)*coef
    gsl_permutation* permutation = gsl_permutation_alloc(orderBSpline);
    gsl_matrix* derMatInv = gsl_matrix_alloc(orderBSpline, orderBSpline);
    int signum1;
    gsl_linalg_LU_decomp(derMat, permutation, &signum1);
    gsl_linalg_LU_invert(derMat, permutation, derMatInv);
    // calculate derMatInv * tmp
    gsl_vector* coefPoly = gsl_vector_alloc(orderBSpline);
    gsl_blas_dgemv(CblasNoTrans, 1.0, derMatInv, tmp, 0.0, coefPoly);
    
    std::vector<double> coefPolyVec = std::vector<double>(coefPoly->data, coefPoly->data + coefPoly->size);
    gsl_vector_free(coefPoly);
    gsl_matrix_free(derMat);
    gsl_matrix_free(derMatInv);
    gsl_permutation_free(permutation);
    gsl_vector_free(tmp);
    gsl_matrix_free(dB);

    return coefPolyVec;
}

std::vector<double> BSplineFit::getfittedPara()
{
    return std::vector<double>(coef->data, coef->data + coef->size);
}

//return in the order of {orderBSpline, dataSize, nBreak, nBasis}
std::vector<unsigned> BSplineFit::getBSplinePara()
{
    return std::vector<unsigned>({orderBSpline, dataSize, nBreak, nBasis});
}

std::pair<double, double> BSplineFit::getxlim()
{
    return std::pair<double, double>(datax[0], datax[dataSize - 1]);
}

std::vector<double> BSplineFit::calculateY(unsigned orderBSpline, unsigned nBreak, 
    std::pair<double, double> xlim, std::vector<double> _coef, std::vector<double> xpts)
{
    gsl_bspline_workspace* bw = gsl_bspline_alloc(orderBSpline, nBreak);
    unsigned nBasis = nBreak - 2 + orderBSpline;
    if (nBasis != _coef.size()) {
        thrower("Error in calculate BSpline: the size of basis function does not match the num of parameter");
    }
    gsl_bspline_knots_uniform(xlim.first,xlim.second, bw);
    gsl_vector* coef = gsl_vector_alloc(_coef.size());
    /* construct the fit matrix X */
    gsl_vector* B = gsl_vector_alloc(nBasis);
    gsl_matrix* X = gsl_matrix_alloc(xpts.size(), nBasis);
    for (unsigned i = 0; i < xpts.size(); ++i)
    {
        /* compute B_j(xi) for all j */
        gsl_bspline_eval(xpts[i], B, bw);
        /* fill in row i of X */
        for (unsigned j = 0; j < nBasis; ++j)
        {
            double Bj = gsl_vector_get(B, j);
            gsl_matrix_set(X, i, j, Bj);
        }
    }
    gsl_vector* tmp = gsl_vector_alloc(nBasis);
    gsl_blas_dgemv(CblasNoTrans, 1.0, X, coef, 0.0, tmp);

    gsl_vector_free(coef);
    gsl_vector_free(B);
    gsl_vector_free(tmp);
    gsl_matrix_free(X);
    gsl_bspline_free(bw);

    return std::vector<double>(tmp->data, tmp->data + tmp->size);
}

bool BSplineFit::isMonotonic(const std::vector<double>& vec)
{
    bool increasing = true;
    bool decreasing = true;
    if (vec.size() == 0) {
        return true;
    }
    else {
        for (int i = 0; i < vec.size() - 1; ++i) {
            if (vec[i] > vec[i + 1])
                increasing = false;
            if (vec[i] < vec[i + 1])
                decreasing = false;
        }
        return increasing || decreasing;
    }
}
