#include "stdafx.h"
#include "PolynomialFit.h"
#include <algorithm>


PolynomialFit::PolynomialFit
(size_t n, double* datax, double* datay, unsigned order, bool useSqrt,
    std::vector<double> init_para,
    size_t max_iter, double ptol, double gtol, double ftol)
    : order(order), useSqrt(useSqrt), numOfPara(1 + order + 2 * useSqrt)
    , max_iter(max_iter), ptol(ptol), gtol(gtol), ftol(ftol)
    , info(-1), rss(-1)
{
    fdf_params = gsl_multifit_nlinear_default_parameters();
    fdf_params.trs = gsl_multifit_nlinear_trs_lmaccel;

    set_data(n, datax, datay, false);
    init_dataFunc();

    fdf.f = &fdf_Helper::invoke_f;
    fdf.df = &fdf_Helper::invoke_df;
    fdf.fvv = &fdf_Helper::invoke_fvv;
    fdf.n = n;
    fdf.p = numOfPara;
    fdf.params = &fit_data;

    covar = gsl_matrix_alloc(numOfPara, numOfPara);
    confid95 = gsl_vector_alloc(numOfPara);
    p0 = gsl_vector_alloc(numOfPara);
    set_initialP(init_para);

    work = gsl_multifit_nlinear_alloc(
        gsl_multifit_nlinear_trust, &fdf_params, n, numOfPara);
    f = gsl_multifit_nlinear_residual(work);
    p = gsl_multifit_nlinear_position(work);
}


double PolynomialFit::polynomial(std::vector<double> para, double t)
{
    double res = 0.0;
    for (unsigned i = 0; i < order + 1; i++)
    {
        res += para[i] * std::pow(t, i);
    }
    if (useSqrt) {
        res += para[order + 1] * std::sqrt(t - para[order + 2]);
    }
    return res;
}

PolynomialFit::~PolynomialFit()
{
    gsl_multifit_nlinear_free(work);
    gsl_vector_free(p0);
    gsl_matrix_free(covar);
}

void PolynomialFit::solve_system()
{
    double rss0, rcond;

    /* initialize solver */
    gsl_multifit_nlinear_init(p0, &fdf, work);

    /* store initial cost */
    gsl_blas_ddot(f, f, &rss0);
    rss0 *= 1.0 / static_cast<double>(fit_data.n - numOfPara);

    /* iterate until convergence */
    gsl_multifit_nlinear_driver(max_iter, ptol, gtol, ftol,
        NULL, NULL, &info, work);

    /* store final cost */
    gsl_blas_ddot(f, f, &rss);
    rss *= 1.0 / static_cast<double>(fit_data.n - numOfPara);
    /* store cond(J(x)) */
    gsl_multifit_nlinear_rcond(&rcond, work);

    /* compute covariance of best fit parameters */
    gsl_multifit_nlinear_covar(gsl_multifit_nlinear_jac(work), 0.0, covar);
    auto covDiag = gsl_matrix_diagonal(covar);
    gsl_vector_swap(confid95, &covDiag.vector);
    double coef95 = gsl_cdf_tdist_Qinv(0.025, fit_data.n - numOfPara);
    std::for_each(confid95->data, confid95->data + confid95->size,
        [this, coef95](auto& tmp) {tmp = coef95 * sqrt(rss * tmp); });
}

void PolynomialFit::set_data(double* datay)
{
    fit_data.y = datay;
}

void PolynomialFit::set_data(size_t n, double* datax, double* datay, bool free)
{
    fit_data.n = n;
    fit_data.x = datax;
    fit_data.y = datay;

    fdf.n = n;
    if (free) { gsl_multifit_nlinear_free(work); }
    work = gsl_multifit_nlinear_alloc(
        gsl_multifit_nlinear_trust, &fdf_params, n, numOfPara);
    f = gsl_multifit_nlinear_residual(work);
    p = gsl_multifit_nlinear_position(work);
}

void PolynomialFit::init_dataFunc()
{
    // can either use std::bind or std::function
    fit_data.f = std::bind(&PolynomialFit::func_f, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    fit_data.df =
        std::function<int(const gsl_vector* p, void* datafit, gsl_matrix* J)>
        ([this](const gsl_vector* p, void* datafit, gsl_matrix* J) {
        return this->func_df(p, datafit, J); });
    fit_data.fvv =
        std::function<int(const gsl_vector* p, const gsl_vector* v, void* datafit, gsl_vector* fvv)>
        ([this](const gsl_vector* p, const gsl_vector* v, void* datafit, gsl_vector* fvv) {
        return this->func_fvv(p, v, datafit, fvv); });
}

void PolynomialFit::set_initialP(std::vector<double> para)
{
    for (unsigned i = 0; i < para.size(); i++)
    {
        gsl_vector_set(p0, i, para[i]);
    }
}

std::vector<double> PolynomialFit::calcFittedGaussian()
{
    std::vector<double> tmp(fit_data.n, 0.0);
    std::vector<double> para(p->data, p->data + p->size);

    size_t i = 0;
    std::for_each(tmp.begin(), tmp.end(), [this, i, para](auto& t1) mutable {
        t1 += polynomial(para, fit_data.x[i]);
        i++; });

    return tmp;
}

std::vector<double> PolynomialFit::fittedPara() const
{
    return std::vector<double>(p->data, p->data + p->size);
}

std::vector<double> PolynomialFit::confidence95Interval() const
{
    return std::vector<double>(confid95->data, confid95->data + confid95->size);
}

//int PolynomialFit::funcf_Wrapper(const gsl_vector* p, void* datafit, gsl_vector* f)
//{
//    data* dataf = static_cast<data*>(datafit);
//    return dataf->func_f(p, datafit, f);
//}

int PolynomialFit::func_f(const gsl_vector* p, void* datafit, gsl_vector* f)
{
    data* dataf = static_cast<data*>(datafit);
    std::vector<double> para(p->data, p->data + p->size);

    for (size_t i = 0; i < dataf->n; ++i)
    {
        double y = polynomial(para, dataf->x[i]);
        gsl_vector_set(f, i, dataf->y[i] - y);
    }

    return GSL_SUCCESS;
}


int PolynomialFit::func_df(const gsl_vector* p, void* datafit, gsl_matrix* J)
{
    struct data* dataf = (struct data*)datafit;
    for (size_t i = 0; i < dataf->n; ++i)
    {
        const double tmp = dataf->x[i];
        /*the minus sign comes from f=yi-gaussian(ti)*/
        for (unsigned j = 0; j < order + 1; j++)
        {
            gsl_matrix_set(J, i, j, -std::pow(tmp, j));
        }
        if (useSqrt) {
            double b0 = gsl_vector_get(p, order + 1);
            double b1 = gsl_vector_get(p, order + 2);
            gsl_matrix_set(J, i, order + 1, -std::sqrt(tmp - b1));
            gsl_matrix_set(J, i, order + 2, b0 / std::sqrt(tmp - b1) / 2);
        }
    }
    return GSL_SUCCESS;
}

int PolynomialFit::func_fvv(const gsl_vector* p, const gsl_vector* v, void* datafit, gsl_vector* fvv)
{
    data* dataf = static_cast<data*>(datafit);
    for (size_t i = 0; i < dataf->n; ++i)
    {
        double res = 0.0;

        if (useSqrt) {
            double tmp = dataf->x[i];
            double b0 = gsl_vector_get(p, order + 1);
            double b1 = gsl_vector_get(p, order + 2);
            double vb0 = gsl_vector_get(v, order + 1);
            double vb1 = gsl_vector_get(v, order + 2);
            res += 1 / std::sqrt(tmp - b1) * vb0 * vb1 + b0 / std::pow(tmp - b1, 1.5) / 4 * vb1 * vb1;
        }
        gsl_vector_set(fvv, i, res);
    }

    return GSL_SUCCESS;
}
