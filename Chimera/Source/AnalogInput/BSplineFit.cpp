#include "stdafx.h"
#include "BSplineFit.h"
#include <gsl/gsl_cdf.h>
#include <gsl/gsl_linalg.h>
#include <algorithm>


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
}

double BSplineFit::calculateY(double x)
{
    if (emptyStart) {
        return 0;
    }
    double yi, yerr;
    gsl_bspline_eval(x, B, bw); // B is assigned values of BSplines at x points of all basis functions
    // yerr is calculated through B^T * cov * B from directly apply covaiance operator to Y, see my oneNote
    gsl_multifit_linear_est(B, coef, cov, &yi, &yerr); 
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
