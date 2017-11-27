
/*
NIED2_MAX_DIMENSION // Min
nied2_state_size // Max
*/

#define NIED2_CHARACTERISTIC 2
#define NIED2_BASE 2
#define NIED2_MAX_DIMENSION 12
#define NIED2_MAX_PRIM_DEGREE 5
#define NIED2_MAX_DEGREE 50
#define NIED2_BIT_COUNT 30
#define NIED2_NBITS (NIED2_BIT_COUNT+1)
#define MAXV (NIED2_NBITS + NIED2_MAX_DEGREE)

#define NIED2_ADD(x,y) (((x)+(y))%2)
#define NIED2_MUL(x,y) (((x)*(y))%2)
#define NIED2_SUB(x,y) NIED2_ADD((x),(y))

static size_t nied2_state_size(unsigned int dimension);
static int nied2_init(void * state, unsigned int dimension);
static int nied2_get(void * state, unsigned int dimension, double * v);

static const int primitive_poly[NIED2_MAX_DIMENSION + 1][NIED2_MAX_PRIM_DEGREE + 1] =
{
  { 1, 0, 0, 0, 0, 0 },
  { 0, 1, 0, 0, 0, 0 },
  { 1, 1, 0, 0, 0, 0 },
  { 1, 1, 1, 0, 0, 0 },
  { 1, 1, 0, 1, 0, 0 },
  { 1, 0, 1, 1, 0, 0 },
  { 1, 1, 0, 0, 1, 0 },
  { 1, 0, 0, 1, 1, 0 },
  { 1, 1, 1, 1, 1, 0 },
  { 1, 0, 1, 0, 0, 1 },
  { 1, 0, 0, 1, 0, 1 },
  { 1, 1, 1, 1, 0, 1 },
  { 1, 1, 1, 0, 1, 1 }
};

static const int poly_degree[NIED2_MAX_DIMENSION + 1] = { 0, 1, 1, 2, 3, 3, 4, 4, 4, 5, 5, 5, 5 };

typedef struct
{
	unsigned int sequence_count;
	int cj[NIED2_NBITS][NIED2_MAX_DIMENSION];
	int nextq[NIED2_MAX_DIMENSION];
}
nied2_state_t;


static size_t nied2_state_size(unsigned int dimension)
{
	return sizeof(nied2_state_t);
}

static void poly_multiply(const int pa[], int pa_degree, const int pb[], int pb_degree, int pc[], int  * pc_degree)
{
	int j, k;
	int pt[NIED2_MAX_DEGREE + 1];
	int pt_degree = pa_degree + pb_degree;

	for (k = 0; k <= pt_degree; k++)
	{
		int term = 0;
		for (j = 0; j <= k; j++)
		{
			const int conv_term = NIED2_MUL(pa[k - j], pb[j]);
			term = NIED2_ADD(term, conv_term);
		}

		pt[k] = term;
	}

	for (k = 0; k <= pt_degree; k++)
		pc[k] = pt[k];

	for (k = pt_degree + 1; k <= NIED2_MAX_DEGREE; k++)
		pc[k] = 0;

	*pc_degree = pt_degree;
}

static void calculate_v(const int px[], int px_degree, int pb[], int * pb_degree, int v[], int maxv)
{
	const int nonzero_element = 1;
	const int arbitrary_element = 1;

	int ph[NIED2_MAX_DEGREE + 1];
	int bigm = *pb_degree;
	int m;
	int r, k, kj;

	for (k = 0; k <= NIED2_MAX_DEGREE; k++)
		ph[k] = pb[k];

	poly_multiply(px, px_degree, pb, *pb_degree, pb, pb_degree);
	m = *pb_degree;
	kj = bigm;

	for (r = 0; r < kj; r++) v[r] = 0;

	v[kj] = 1;

	if (kj >= bigm)
	{
		for (r = kj + 1; r < m; r++)
			v[r] = arbitrary_element;
	}
	else
	{
		int term = NIED2_SUB(0, ph[kj]);

		for (r = kj + 1; r < bigm; r++)
		{
			v[r] = arbitrary_element;
			term = NIED2_SUB(term, NIED2_MUL(ph[r], v[r]));
		}

		v[bigm] = NIED2_ADD(nonzero_element, term);

		for (r = bigm + 1; r < m; r++)
			v[r] = arbitrary_element;
	}

	for (r = 0; r <= maxv - m; r++)
	{
		int term = 0;
		for (k = 0; k < m; k++) {
			term = NIED2_SUB(term, NIED2_MUL(pb[k], v[r + k]));
		}

		v[r + m] = term;
	}
}


static void calculate_cj(nied2_state_t * ns, unsigned int dimension)
{
	int ci[NIED2_NBITS][NIED2_NBITS];
	int v[MAXV + 1];
	int r;
	unsigned int i_dim;

	for (i_dim = 0; i_dim < dimension; i_dim++)
	{

		const int poly_index = i_dim + 1;
		int j, k;
		int u = 0;
		int pb[NIED2_MAX_DEGREE + 1];
		int px[NIED2_MAX_DEGREE + 1];
		int px_degree = poly_degree[poly_index];
		int pb_degree = 0;

		for (k = 0; k <= px_degree; k++)
		{
			px[k] = primitive_poly[poly_index][k];
			pb[k] = 0;
		}

		for (; k < NIED2_MAX_DEGREE + 1; k++)
		{
			px[k] = 0;
			pb[k] = 0;
		}

		pb[0] = 1;

		for (j = 0; j < NIED2_NBITS; j++)
		{
			if (u == 0) calculate_v(px, px_degree, pb, &pb_degree, v, MAXV);
			for (r = 0; r < NIED2_NBITS; r++)
				ci[r][j] = v[r + u];
			++u;
			if (u == px_degree) u = 0;
		}

		for (r = 0; r < NIED2_NBITS; r++)
		{
			int term = 0;
			for (j = 0; j < NIED2_NBITS; j++)
				term = 2 * term + ci[r][j];
			ns->cj[r][i_dim] = term;
		}

	}
}

static int nied2_init(void * state, unsigned int dimension)
{
	nied2_state_t * n_state = (nied2_state_t *)state;
	unsigned int i_dim;
	if (dimension < 1 || dimension > NIED2_MAX_DIMENSION) return GSL_EINVAL;
	calculate_cj(n_state, dimension);
	for (i_dim = 0; i_dim < dimension; i_dim++) n_state->nextq[i_dim] = 0;
	n_state->sequence_count = 0;
	return 0;
}

static int nied2_get(void * state, unsigned int dimension, double * v)
{
	static const double recip = 1.0 / (double)(1U << NIED2_NBITS);
	nied2_state_t * n_state = (nied2_state_t *)state;
	int r;
	int c;
	unsigned int i_dim;

	for (i_dim = 0; i_dim < dimension; i_dim++)
		v[i_dim] = n_state->nextq[i_dim] * recip;

	r = 0;
	c = n_state->sequence_count;

	while (1)
	{
		if ((c % 2) == 1)
		{
			++r;
			c /= 2;
		}
		else break;
	}

	if (r >= NIED2_NBITS) return GSL_EFAILED;

	for (i_dim = 0; i_dim < dimension; i_dim++)
		n_state->nextq[i_dim] ^= n_state->cj[r][i_dim];

	n_state->sequence_count++;

	return 0;
}
