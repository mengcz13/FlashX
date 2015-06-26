#include <stdio.h>
#include <cblas.h>

#include "vector.h"
#include "mem_worker_thread.h"
#include "dense_matrix.h"
#include "mem_matrix_store.h"

using namespace fm;

class set_col_operate: public type_set_operate<int>
{
	size_t num_cols;
public:
	set_col_operate(size_t num_cols) {
		this->num_cols = num_cols;
	}

	void set(int *arr, size_t num_eles, off_t row_idx, off_t col_idx) const {
		for (size_t i = 0; i < num_eles; i++) {
			arr[i] = (row_idx + i) * num_cols + col_idx;
		}
	}
};

class set_row_operate: public type_set_operate<int>
{
	size_t num_cols;
public:
	set_row_operate(size_t num_cols) {
		this->num_cols = num_cols;
	}

	void set(int *arr, size_t num_eles, off_t row_idx, off_t col_idx) const {
		for (size_t i = 0; i < num_eles; i++) {
			arr[i] = row_idx * num_cols + col_idx + i;
		}
	}
};

class set_col_long_operate: public type_set_operate<size_t>
{
	size_t num_cols;
public:
	set_col_long_operate(size_t num_cols) {
		this->num_cols = num_cols;
	}

	void set(size_t *arr, size_t num_eles, off_t row_idx, off_t col_idx) const {
		for (size_t i = 0; i < num_eles; i++) {
			arr[i] = (row_idx + i) * num_cols + col_idx;
		}
	}
};

class set_row_long_operate: public type_set_operate<size_t>
{
	size_t num_cols;
public:
	set_row_long_operate(size_t num_cols) {
		this->num_cols = num_cols;
	}

	void set(size_t *arr, size_t num_eles, off_t row_idx, off_t col_idx) const {
		for (size_t i = 0; i < num_eles; i++) {
			arr[i] = row_idx * num_cols + col_idx + i;
		}
	}
};

size_t long_dim = 10000000;

/*
 * This is a naive implementation of matrix multiplication.
 * It should be correct
 */
dense_matrix::ptr naive_multiply(const dense_matrix &m1, const dense_matrix &m2)
{
	m1.materialize_self();
	m2.materialize_self();
	detail::mem_matrix_store::ptr res_store = detail::mem_matrix_store::create(
			m1.get_num_rows(), m2.get_num_cols(), matrix_layout_t::L_ROW,
			get_scalar_type<int>(), -1);
	const detail::mem_matrix_store &mem_m1
		= dynamic_cast<const detail::mem_matrix_store &>(m1.get_data());
	const detail::mem_matrix_store &mem_m2
		= dynamic_cast<const detail::mem_matrix_store &>(m2.get_data());
#pragma omp parallel for
	for (size_t i = 0; i < m1.get_num_rows(); i++) {
		for (size_t j = 0; j < m2.get_num_cols(); j++) {
			int sum = 0;
			for (size_t k = 0; k < m1.get_num_cols(); k++) {
				sum += mem_m1.get<int>(i, k) * mem_m2.get<int>(k, j);
			}
			res_store->set<int>(i, j, sum);
		}
	}
	return dense_matrix::create(res_store);
}

void verify_result(const dense_matrix &m1, const dense_matrix &m2)
{
	assert(m1.get_num_rows() == m2.get_num_rows());
	assert(m1.get_num_cols() == m2.get_num_cols());

	m1.materialize_self();
	m2.materialize_self();
	const detail::mem_matrix_store &mem_m1
		= dynamic_cast<const detail::mem_matrix_store &>(m1.get_data());
	const detail::mem_matrix_store &mem_m2
		= dynamic_cast<const detail::mem_matrix_store &>(m2.get_data());
#pragma omp parallel for
	for (size_t i = 0; i < m1.get_num_rows(); i++)
		for (size_t j = 0; j < m1.get_num_cols(); j++)
			assert(mem_m1.get<int>(i, j) == mem_m2.get<int>(i, j));
}

enum matrix_val_t
{
	DEFAULT,
	SEQ,
	NUM_TYPES,
} matrix_val;

dense_matrix::ptr create_seq_matrix(size_t nrow, size_t ncol,
		matrix_layout_t layout, int num_nodes, const scalar_type &type)
{
	if (type == get_scalar_type<int>()) {
		if (layout == matrix_layout_t::L_COL)
			return dense_matrix::create(nrow, ncol, layout,
					type, set_col_operate(ncol), num_nodes, true);
		else
			return dense_matrix::create(nrow, ncol, layout,
					type, set_row_operate(ncol), num_nodes, true);
	}
	else if (type == get_scalar_type<size_t>()) {
		if (layout == matrix_layout_t::L_COL)
			return dense_matrix::create(nrow, ncol, layout,
					type, set_col_long_operate(ncol), num_nodes, true);
		else
			return dense_matrix::create(nrow, ncol, layout,
					type, set_row_long_operate(ncol), num_nodes, true);
	}
	else
		return dense_matrix::ptr();
}

dense_matrix::ptr create_matrix(size_t nrow, size_t ncol,
		matrix_layout_t layout, int num_nodes,
		const scalar_type &type = get_scalar_type<int>())
{
	switch (matrix_val) {
		case matrix_val_t::DEFAULT:
			if (layout == matrix_layout_t::L_COL)
				return dense_matrix::create(nrow, ncol, layout,
						type, num_nodes, true);
			else
				return dense_matrix::create(nrow, ncol, layout,
						type, num_nodes, true);
		case matrix_val_t::SEQ:
			return create_seq_matrix(nrow, ncol, layout, num_nodes, type);
		default:
			assert(0);
			return dense_matrix::ptr();
	}
}

void test_multiply_scalar(int num_nodes)
{
	printf("Test scalar multiplication\n");
	dense_matrix::ptr orig = create_matrix(long_dim, 10,
			matrix_layout_t::L_COL, num_nodes);
	dense_matrix::ptr res = orig->multiply_scalar(10);
	res->materialize_self();
	orig->materialize_self();
	const detail::mem_matrix_store &orig_store1
		= dynamic_cast<const detail::mem_matrix_store &>(orig->get_data());
	const detail::mem_matrix_store &res_store1
		= dynamic_cast<const detail::mem_matrix_store &>(res->get_data());
#pragma omp parallel for
	for (size_t i = 0; i < res_store1.get_num_rows(); i++)
		for (size_t j = 0; j < res_store1.get_num_cols(); j++)
			assert(res_store1.get<int>(i, j) == orig_store1.get<int>(i, j) * 10);

	orig = create_matrix(long_dim, 10, matrix_layout_t::L_ROW, num_nodes);
	res = orig->multiply_scalar(10);
	res->materialize_self();
	orig->materialize_self();
	const detail::mem_matrix_store &orig_store2
		= dynamic_cast<const detail::mem_matrix_store &>(orig->get_data());
	const detail::mem_matrix_store &res_store2
		= dynamic_cast<const detail::mem_matrix_store &>(res->get_data());
#pragma omp parallel for
	for (size_t i = 0; i < res_store2.get_num_rows(); i++)
		for (size_t j = 0; j < res_store2.get_num_cols(); j++)
			assert(res_store2.get<int>(i, j) == orig_store2.get<int>(i, j) * 10);
}

void test_ele_wise(int num_nodes)
{
	printf("Test element-wise operations\n");
	dense_matrix::ptr m1 = create_matrix(long_dim, 10,
			matrix_layout_t::L_COL, num_nodes);
	dense_matrix::ptr m2 = create_matrix(long_dim, 10,
			matrix_layout_t::L_COL, num_nodes);
	dense_matrix::ptr res = m1->add(*m2);
	res->materialize_self();
	m1->materialize_self();
	const detail::mem_matrix_store &res_store
		= dynamic_cast<const detail::mem_matrix_store &>(res->get_data());
	const detail::mem_matrix_store &m1_store
		= dynamic_cast<const detail::mem_matrix_store &>(m1->get_data());
#pragma omp parallel for
	for (size_t i = 0; i < res_store.get_num_rows(); i++)
		for (size_t j = 0; j < res_store.get_num_cols(); j++)
			assert(res_store.get<int>(i, j) == m1_store.get<int>(i, j) * 2);
}

void test_multiply_col(int num_nodes)
{
	printf("Test multiplication on tall matrix stored column wise\n");
	dense_matrix::ptr m1 = create_matrix(long_dim, 10,
			matrix_layout_t::L_COL, num_nodes);
	dense_matrix::ptr m2 = create_matrix(10, 9,
			matrix_layout_t::L_COL, num_nodes);
	dense_matrix::ptr correct = naive_multiply(*m1, *m2);

	printf("Test multiply on col_matrix\n");
	dense_matrix::ptr res1 = m1->multiply(*m2);
	verify_result(*res1, *correct);
}

void test_agg_col(int num_nodes)
{
	printf("Test aggregation on tall matrix stored column wise\n");
	dense_matrix::ptr m1 = create_matrix(long_dim, 10,
			matrix_layout_t::L_COL, num_nodes, get_scalar_type<size_t>());
	const bulk_operate &op
		= m1->get_type().get_basic_ops().get_add();
	scalar_variable::ptr res = m1->aggregate(op);
	assert(res->get_type() == m1->get_type());
	assert(res->get_type() == get_scalar_type<size_t>());
	size_t sum = *(size_t *) res->get_raw();
	size_t num_eles = m1->get_num_rows() * m1->get_num_cols();
	if (matrix_val == matrix_val_t::DEFAULT)
		assert(sum == 0);
	else
		assert(sum == (num_eles - 1) * num_eles / 2);
}

void test_multiply_matrix(int num_nodes)
{
	dense_matrix::ptr m1, m2, correct, res;

	printf("Test multiplication on wide row matrix X tall column matrix\n");
	m1 = create_matrix(10, long_dim, matrix_layout_t::L_ROW, num_nodes);
	m2 = create_matrix(long_dim, 9, matrix_layout_t::L_COL, num_nodes);
	correct = naive_multiply(*m1, *m2);
	res = m1->multiply(*m2);
	verify_result(*res, *correct);

	printf("Test multiplication on wide row matrix X tall row matrix\n");
	m1 = create_matrix(10, long_dim, matrix_layout_t::L_ROW, num_nodes);
	m2 = create_matrix(long_dim, 9, matrix_layout_t::L_ROW, num_nodes);
	correct = naive_multiply(*m1, *m2);
	res = m1->multiply(*m2);
	verify_result(*res, *correct);

	printf("Test multiplication on wide column matrix X tall column matrix\n");
	m1 = create_matrix(10, long_dim, matrix_layout_t::L_COL, num_nodes);
	m2 = create_matrix(long_dim, 9, matrix_layout_t::L_COL, num_nodes);
	correct = naive_multiply(*m1, *m2);
	res = m1->multiply(*m2);
	verify_result(*res, *correct);

	printf("Test multiplication on wide column matrix X tall row matrix\n");
	m1 = create_matrix(10, long_dim, matrix_layout_t::L_COL, num_nodes);
	m2 = create_matrix(long_dim, 9, matrix_layout_t::L_ROW, num_nodes);
	correct = naive_multiply(*m1, *m2);
	res = m1->multiply(*m2);
	verify_result(*res, *correct);

	printf("Test multiplication on tall row matrix X small row matrix\n");
	m1 = create_matrix(long_dim, 10, matrix_layout_t::L_ROW, num_nodes);
	m2 = create_matrix(10, 9, matrix_layout_t::L_ROW, num_nodes);
	correct = naive_multiply(*m1, *m2);
	res = m1->multiply(*m2);
	verify_result(*res, *correct);

	printf("Test multiplication on tall row matrix X small column matrix\n");
	m1 = create_matrix(long_dim, 10, matrix_layout_t::L_ROW, num_nodes);
	m2 = create_matrix(10, 9, matrix_layout_t::L_COL, num_nodes);
	correct = naive_multiply(*m1, *m2);
	res = m1->multiply(*m2);
	verify_result(*res, *correct);

	printf("Test multiplication on tall column matrix X small row matrix\n");
	m1 = create_matrix(long_dim, 10, matrix_layout_t::L_COL, num_nodes);
	m2 = create_matrix(10, 9, matrix_layout_t::L_ROW, num_nodes);
	correct = naive_multiply(*m1, *m2);
	res = m1->multiply(*m2);
	verify_result(*res, *correct);

	printf("Test multiplication on tall column matrix X small column matrix\n");
	m1 = create_matrix(long_dim, 10, matrix_layout_t::L_COL, num_nodes);
	m2 = create_matrix(10, 9, matrix_layout_t::L_COL, num_nodes);
	correct = naive_multiply(*m1, *m2);
	res = m1->multiply(*m2);
	verify_result(*res, *correct);
}

void test_agg_row(int num_nodes)
{
	printf("Test aggregation on tall matrix stored row wise\n");
	dense_matrix::ptr m1 = create_matrix(long_dim, 10,
			matrix_layout_t::L_ROW, num_nodes, get_scalar_type<size_t>());
	const bulk_operate &op
		= m1->get_type().get_basic_ops().get_add();
	scalar_variable::ptr res = m1->aggregate(op);
	assert(res->get_type() == m1->get_type());
	size_t sum = *(size_t *) res->get_raw();
	size_t num_eles = m1->get_num_rows() * m1->get_num_cols();
	if (matrix_val == matrix_val_t::DEFAULT)
		assert(sum == 0);
	else
		assert(sum == (num_eles - 1) * num_eles / 2);
}

void test_agg_sub_col(int num_nodes)
{
	printf("Test aggregation on a column-wise submatrix\n");
	dense_matrix::ptr col_m = create_matrix(long_dim, 10,
			matrix_layout_t::L_COL, num_nodes, get_scalar_type<size_t>());
	std::vector<off_t> idxs(3);
	idxs[0] = 1;
	idxs[1] = 5;
	idxs[2] = 3;
	dense_matrix::ptr sub_m = col_m->get_cols(idxs);
	assert(sub_m != NULL);

	const bulk_operate &op = sub_m->get_type().get_basic_ops().get_add();
	scalar_variable::ptr res = sub_m->aggregate(op);
	assert(res->get_type() == sub_m->get_type());
	size_t sum = *(size_t *) res->get_raw();
	size_t ncol = col_m->get_num_cols();
	size_t nrow = col_m->get_num_rows();
	size_t sub_ncol = sub_m->get_num_cols();
	size_t expected = sub_ncol * ncol * (nrow - 1) * nrow / 2;
	for (size_t i = 0; i < idxs.size(); i++)
		expected += idxs[i] * nrow;
	if (matrix_val == matrix_val_t::DEFAULT)
		assert(sum == 0);
	else
		assert(sum == expected);
}

void test_agg_sub_row(int num_nodes)
{
	printf("Test aggregation on a row-wise submatrix\n");
	dense_matrix::ptr col_m = create_matrix(long_dim, 10,
			matrix_layout_t::L_COL, num_nodes, get_scalar_type<size_t>());
	std::vector<off_t> idxs(3);
	idxs[0] = 1;
	idxs[1] = 5;
	idxs[2] = 3;
	dense_matrix::ptr sub_col_m = col_m->get_cols(idxs);
	dense_matrix::ptr sub_row_m = sub_col_m->transpose();

	const bulk_operate &op = sub_col_m->get_type().get_basic_ops().get_add();
	scalar_variable::ptr col_res = sub_col_m->aggregate(op);
	assert(col_res->get_type() == sub_col_m->get_type());
	scalar_variable::ptr row_res = sub_row_m->aggregate(op);
	assert(row_res->get_type() == sub_row_m->get_type());
	assert(*(size_t *) col_res->get_raw() == *(size_t *) row_res->get_raw());
}

#if 0

void test_conv_row_col()
{
	printf("test conv col-wise to row-wise, row-wise to col-wise\n");
	I_mem_dense_matrix::ptr m1 = I_mem_dense_matrix::create(10000, 10,
			matrix_layout_t::L_COL, set_col_operate(10));
	mem_col_dense_matrix::ptr col_m
		= mem_col_dense_matrix::cast(m1->get_matrix());
	mem_row_dense_matrix::ptr row_m = col_m->get_row_store();
	I_mem_dense_matrix::ptr c1 = I_mem_dense_matrix::create(col_m);
	I_mem_dense_matrix::ptr c2 = I_mem_dense_matrix::create(row_m);
	for (size_t i = 0; i < col_m->get_num_rows(); i++) {
		for (size_t j = 0; j < col_m->get_num_cols(); j++)
			assert(c1->get(i, j) == c2->get(i, j));
	}
	col_m = row_m->get_col_store();
	c1 = I_mem_dense_matrix::create(col_m);
	c2 = I_mem_dense_matrix::create(row_m);
	for (size_t i = 0; i < col_m->get_num_rows(); i++)
		for (size_t j = 0; j < col_m->get_num_cols(); j++)
			assert(c1->get(i, j) == c2->get(i, j));
}

void test_rand_init()
{
	printf("test rand init\n");
	dense_matrix::ptr m = dense_matrix::create_rand<double>(-1.0, 1.0,
			long_dim / 100, 10, matrix_layout_t::L_COL);
	double sum = 0;
	const detail::mem_matrix_store &mem_m
		= dynamic_cast<const detail::mem_matrix_store &>(*m);
	for (size_t i = 0; i < m->get_num_rows(); i++)
		for (size_t j = 0; j < m->get_num_cols(); j++) {
			double v = mem_m.get<double>(i, j);
			assert(v >= -1.0 && v <= 1.0);
			sum += v;
		}
	printf("sum: %f\n", sum);
}

void test_flatten()
{
	printf("test flatten a matrix to a vector\n");
	I_mem_dense_matrix::ptr m = I_mem_dense_matrix::create(10000, 10,
			matrix_layout_t::L_COL, set_col_operate(10));
	mem_vector::ptr vec = m->get_matrix()->flatten(true);
	for (size_t i = 0; i < vec->get_length(); i++)
		assert((size_t) vec->get<int>(i) == i);

	m = I_mem_dense_matrix::create(10000, 10, matrix_layout_t::L_ROW,
			set_row_operate(10));
	vec = m->get_matrix()->flatten(true);
	for (size_t i = 0; i < vec->get_length(); i++)
		assert((size_t) vec->get<int>(i) == i);

	m = I_mem_dense_matrix::create(10000, 10, matrix_layout_t::L_COL,
			set_col_operate(10));
	vec = m->get_matrix()->flatten(false);
	for (size_t i = 0; i < vec->get_length(); i++) {
		size_t row_idx = i % m->get_num_rows();
		size_t col_idx = i / m->get_num_rows();
		assert((size_t) vec->get<int>(i) == row_idx * m->get_num_cols() + col_idx);
	}

	m = I_mem_dense_matrix::create(10000, 10, matrix_layout_t::L_ROW,
			set_row_operate(10));
	vec = m->get_matrix()->flatten(false);
	for (size_t i = 0; i < vec->get_length(); i++) {
		size_t row_idx = i % m->get_num_rows();
		size_t col_idx = i / m->get_num_rows();
		assert((size_t) vec->get<int>(i) == row_idx * m->get_num_cols() + col_idx);
	}
}

#endif

void test_scale_cols1(dense_matrix::ptr orig)
{
	vector::ptr vals = create_vector<int>(0, orig->get_num_cols() - 1, 1);
	dense_matrix::ptr res = orig->scale_cols(vals);
	res->materialize_self();
	orig->materialize_self();
	const detail::mem_matrix_store &orig_store1
		= dynamic_cast<const detail::mem_matrix_store &>(orig->get_data());
	const detail::mem_matrix_store &res_store1
		= dynamic_cast<const detail::mem_matrix_store &>(res->get_data());
	const detail::smp_vec_store &val_store
		= dynamic_cast<const detail::smp_vec_store &>(vals->get_data());
#pragma omp parallel for
	for (size_t i = 0; i < res_store1.get_num_rows(); i++)
		for (size_t j = 0; j < res_store1.get_num_cols(); j++)
			assert(res_store1.get<int>(i, j)
					== orig_store1.get<int>(i, j) * val_store.get<int>(j));
}

void test_scale_cols(int num_nodes)
{
	printf("Test scale cols of tall column matrix\n");
	dense_matrix::ptr orig = create_matrix(long_dim, 10,
			matrix_layout_t::L_COL, num_nodes);
	test_scale_cols1(orig);

	printf("Test scale cols of tall row matrix\n");
	orig = create_matrix(long_dim, 10, matrix_layout_t::L_ROW, num_nodes);
	test_scale_cols1(orig);

	printf("Test scale cols of wide column matrix\n");
	orig = create_matrix(10, long_dim, matrix_layout_t::L_COL, num_nodes);
	test_scale_cols1(orig);

	printf("Test scale cols of wide row matrix\n");
	orig = create_matrix(10, long_dim, matrix_layout_t::L_ROW, num_nodes);
	test_scale_cols1(orig);
}

void test_scale_rows1(dense_matrix::ptr orig)
{
	vector::ptr vals = create_vector<int>(0, orig->get_num_rows() - 1, 1);
	dense_matrix::ptr res = orig->scale_rows(vals);
	res->materialize_self();
	orig->materialize_self();
	const detail::mem_matrix_store &orig_store1
		= dynamic_cast<const detail::mem_matrix_store &>(orig->get_data());
	const detail::mem_matrix_store &res_store1
		= dynamic_cast<const detail::mem_matrix_store &>(res->get_data());
	const detail::smp_vec_store &val_store
		= dynamic_cast<const detail::smp_vec_store &>(vals->get_data());
#pragma omp parallel for
	for (size_t i = 0; i < res_store1.get_num_rows(); i++)
		for (size_t j = 0; j < res_store1.get_num_cols(); j++)
			assert(res_store1.get<int>(i, j)
					== orig_store1.get<int>(i, j) * val_store.get<int>(i));
}

void test_scale_rows(int num_nodes)
{
	printf("Test scale rows of wide row matrix\n");
	dense_matrix::ptr orig = create_matrix(10, long_dim,
			matrix_layout_t::L_ROW, num_nodes);
	test_scale_rows1(orig);

	printf("Test scale rows of wide column matrix\n");
	orig = create_matrix(10, long_dim, matrix_layout_t::L_COL, num_nodes);
	test_scale_rows1(orig);

	printf("Test scale rows of tall row matrix\n");
	orig = create_matrix(long_dim, 10, matrix_layout_t::L_ROW, num_nodes);
	test_scale_rows1(orig);

	printf("Test scale rows of tall column matrix\n");
	orig = create_matrix(long_dim, 10, matrix_layout_t::L_COL, num_nodes);
	test_scale_rows1(orig);
}

#if 0
void test_create_const()
{
	printf("test create const matrix\n");
	dense_matrix::ptr mat = dense_matrix::create(10000, 10,
			matrix_layout_t::L_COL, get_scalar_type<int>());
	for (size_t i = 0; i < mat->get_num_rows(); i++)
		for (size_t j = 0; j < mat->get_num_cols(); j++)
			assert(mat->get<int>(i, j) == 0);

	mat = mem_dense_matrix::create_const<int>(1, 10000, 10,
			matrix_layout_t::L_COL);
	for (size_t i = 0; i < mat->get_num_rows(); i++)
		for (size_t j = 0; j < mat->get_num_cols(); j++)
			assert(mat->get<int>(i, j) == 1);
}
#endif

class sum_apply_op: public arr_apply_operate
{
public:
	sum_apply_op(): arr_apply_operate(1) {
	}

	void run(const local_vec_store &in, local_vec_store &out) const {
		assert(in.get_type() == get_scalar_type<int>());
		assert(out.get_type() == get_scalar_type<long>());
		long res = 0;
		for (size_t i = 0; i < in.get_length(); i++)
			res += in.get<int>(i);
		out.set<long>(0, res);
	}

	const scalar_type &get_input_type() const {
		return get_scalar_type<int>();
	}

	const scalar_type &get_output_type() const {
		return get_scalar_type<long>();
	}
};

void test_apply1(dense_matrix::ptr mat)
{
	size_t num_rows = mat->get_num_rows();
	size_t num_cols = mat->get_num_cols();
	dense_matrix::ptr res;
	vector::ptr res_vec;

	printf("Test apply on rows of a %s %s-wise matrix\n",
			mat->is_wide() ? "wide" : "tall",
			mat->store_layout() == matrix_layout_t::L_ROW ? "row" : "column");
	res = mat->apply(apply_margin::MAR_ROW,
			arr_apply_operate::const_ptr(new sum_apply_op()));
	assert(res->get_num_cols() == 1 && res->get_num_rows() == mat->get_num_rows());
	assert(res->is_type<long>());
	res->materialize_self();
	res_vec = res->get_col(0);
	const detail::smp_vec_store &vstore1
		= dynamic_cast<const detail::smp_vec_store &>(res_vec->get_data());
	for (size_t i = 0; i < res_vec->get_length(); i++)
		assert(vstore1.get<long>(i)
				== i * num_cols * num_cols + (num_cols - 1) * num_cols / 2);

	printf("Test apply on columns of a %s %s-wise matrix\n",
			mat->is_wide() ? "wide" : "tall",
			mat->store_layout() == matrix_layout_t::L_ROW ? "row" : "column");
	res = mat->apply(apply_margin::MAR_COL,
			arr_apply_operate::const_ptr(new sum_apply_op()));
	assert(res->get_num_rows() == 1 && res->get_num_cols() == mat->get_num_cols());
	assert(res->is_type<long>());
	res->materialize_self();
	res_vec = res->get_row(0);
	const detail::smp_vec_store &vstore2
		= dynamic_cast<const detail::smp_vec_store &>(res_vec->get_data());
	for (size_t i = 0; i < res_vec->get_length(); i++)
		assert(vstore2.get<long>(i)
				== (num_rows - 1) * num_rows / 2 * num_cols + num_rows * i);
}

void test_apply()
{
	detail::mem_matrix_store::ptr store;
	dense_matrix::ptr mat;

	// Tall row-wise matrix
	store = detail::mem_matrix_store::create(long_dim, 10,
			matrix_layout_t::L_ROW, get_scalar_type<int>(), -1);
	store->set_data(set_row_operate(store->get_num_cols()));
	mat = dense_matrix::create(store);
	test_apply1(mat);

	// Tall col-wise matrix
	store = detail::mem_matrix_store::create(long_dim, 10,
			matrix_layout_t::L_COL, get_scalar_type<int>(), -1);
	store->set_data(set_col_operate(store->get_num_cols()));
	mat = dense_matrix::create(store);
	test_apply1(mat);

	// Wide row-wise matrix
	store = detail::mem_matrix_store::create(10, long_dim,
			matrix_layout_t::L_ROW, get_scalar_type<int>(), -1);
	store->set_data(set_row_operate(store->get_num_cols()));
	mat = dense_matrix::create(store);
	test_apply1(mat);

	// wide col-wise matrix
	store = detail::mem_matrix_store::create(10, long_dim,
			matrix_layout_t::L_COL, get_scalar_type<int>(), -1);
	store->set_data(set_col_operate(store->get_num_cols()));
	mat = dense_matrix::create(store);
	test_apply1(mat);
}

void test_conv_vec2mat()
{
	printf("convert a vector to a matrix\n");
	size_t len = 10000;
	size_t num_rows = 1000;
	vector::ptr vec = create_vector<int>(0, len, 1);
	dense_matrix::ptr mat = vec->conv2mat(num_rows, len / num_rows, true);
	assert(mat);
	assert(mat->store_layout() == matrix_layout_t::L_ROW);

	mat = vec->conv2mat(num_rows, len / num_rows, false);
	assert(mat);
	assert(mat->store_layout() == matrix_layout_t::L_COL);
}

void test_write2file1(detail::mem_matrix_store::ptr mat)
{
	char *tmp_file_name = tempnam(".", "tmp.mat");
	if (mat->store_layout() == matrix_layout_t::L_ROW)
		mat->set_data(set_row_operate(mat->get_num_cols()));
	else
		mat->set_data(set_col_operate(mat->get_num_cols()));
	bool ret = mat->write2file(tmp_file_name);
	assert(ret);

	detail::mem_matrix_store::ptr read_mat = detail::mem_matrix_store::load(tmp_file_name);
	assert(read_mat);
	assert(read_mat->get_num_rows() == mat->get_num_rows());
	assert(read_mat->get_num_cols() == mat->get_num_cols());
	assert(read_mat->get_type() == mat->get_type());
	assert(read_mat->store_layout() == mat->store_layout());
	for (size_t i = 0; i < mat->get_num_rows(); i++) {
		for (size_t j = 0; j < mat->get_num_cols(); j++)
			assert(mat->get<int>(i, j) == read_mat->get<int>(i, j));
	}

	unlink(tmp_file_name);
}

void test_write2file()
{
	detail::mem_matrix_store::ptr mat;
	printf("write a tall row matrix\n");
	mat = detail::mem_matrix_store::create(1000000, 10,
			matrix_layout_t::L_ROW, get_scalar_type<int>(), -1);
	test_write2file1(mat);
	printf("write a tall column matrix\n");
	mat = detail::mem_matrix_store::create(1000000, 10,
			matrix_layout_t::L_COL, get_scalar_type<int>(), -1);
	test_write2file1(mat);
}

void test_cast()
{
	printf("test cast type\n");
	dense_matrix::ptr mat, mat1;

	{
		mat = dense_matrix::create_rand<int>(
				0, 1000, 100000, 10, matrix_layout_t::L_ROW);
		mat1 = mat->cast_ele_type(get_scalar_type<long>());
		mat1->materialize_self();
		const detail::mem_matrix_store &mem_mat
			= dynamic_cast<const detail::mem_matrix_store &>(mat->get_data());
		const detail::mem_matrix_store &mem_mat1
			= dynamic_cast<const detail::mem_matrix_store &>(mat1->get_data());
#pragma omp parallel for
		for (size_t i = 0; i < mat->get_num_rows(); i++)
			for (size_t j = 0; j < mat->get_num_cols(); j++)
				assert(mem_mat.get<int>(i, j) == mem_mat1.get<long>(i, j));
	}

	{
		mat = dense_matrix::create_rand<float>(
				0, 1000, 100000, 10, matrix_layout_t::L_ROW);
		mat1 = mat->cast_ele_type(get_scalar_type<double>());
		mat1->materialize_self();
		const detail::mem_matrix_store &mem_mat
			= dynamic_cast<const detail::mem_matrix_store &>(mat->get_data());
		const detail::mem_matrix_store &mem_mat1
			= dynamic_cast<const detail::mem_matrix_store &>(mat1->get_data());
#pragma omp parallel for
		for (size_t i = 0; i < mat->get_num_rows(); i++)
			for (size_t j = 0; j < mat->get_num_cols(); j++)
				assert(mem_mat.get<float>(i, j) == mem_mat1.get<double>(i, j));
	}
}

int main(int argc, char *argv[])
{
	int num_nodes = 1;
	int num_threads = 8;
	if (argc >= 3) {
		num_nodes = atoi(argv[1]);
		num_threads = atoi(argv[2]);
	}
	detail::mem_thread_pool::init_global_mem_threads(num_nodes,
			num_threads / num_nodes);

	test_cast();
	test_write2file();
#if 0
	test_create_const();
#endif
	test_apply();
	test_conv_vec2mat();

	for (int i = 0; i < matrix_val_t::NUM_TYPES; i++) {
		matrix_val = (matrix_val_t) i;
		printf("matrix val type: %d\n", i);

		test_scale_cols(-1);
		test_scale_cols(num_nodes);
		test_scale_rows(-1);
		test_scale_rows(num_nodes);
		test_multiply_scalar(-1);
		test_multiply_scalar(num_nodes);
		test_ele_wise(-1);
		test_ele_wise(num_nodes);
		test_multiply_col(-1);
		test_multiply_col(num_nodes);
		test_agg_col(-1);
		test_agg_col(num_nodes);
		test_multiply_matrix(-1);
		test_multiply_matrix(num_nodes);
		test_agg_row(-1);
		test_agg_row(num_nodes);
		test_agg_sub_col(-1);
		test_agg_sub_row(-1);
#if 0
		test_rand_init();
		test_conv_row_col();
		test_flatten();
#endif
	}
}
