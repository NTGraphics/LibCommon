//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//    .--------------------------------------------------.
//    |  This file is part of NTCodeBase                 |
//    |  Created 2018 by NT (https://ttnghia.github.io)  |
//    '--------------------------------------------------'
//                            \o/
//                             |
//                            / |
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

#include <LibCommon/LinearAlgebra/ImplicitQRSVD.h>
#include <LibCommon/Math/MathHelpers.h>

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
namespace NTCodeBase::QRSVD {
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

/**
   Class for givens rotation.
   Row rotation G*A corresponds to something like
   c -s  0
   ( s  c  0 ) A
   0  0  1
   Column rotation A G' corresponds to something like
   c -s  0
   A ( s  c  0 )
   0  0  1

   c and s are always computed so that
   ( c -s ) ( a )  =  ( * )
   s  c     b       ( 0 )

   Assume rowi < rowk.
 */
template<class T>
class GivensRotation {
public:
    Int rowi;
    Int rowk;
    T   c;
    T   s;

    GivensRotation(Int rowi_, Int rowk_) : rowi(rowi_), rowk(rowk_), c(1), s(0) {}
    GivensRotation(T a, T b, Int rowi_, Int rowk_) : rowi(rowi_), rowk(rowk_) { compute(a, b); }

    void transposeInPlace() { s = -s; }

    /**
       Compute c and s from a and b so that
       ( c -s ) ( a )  =  ( * )
       s  c     b       ( 0 )
     */
    void compute(const T a, const T b) {
        T d = a * a + b * b;
        c = T(1.0);
        s = T(0);
        if(d != 0) {
            // T t = 1 / std::sqrt(d);
            T t = MathHelpers::approx_rsqrt(d);
            c = a * t;
            s = -b * t;
        }
    }

    /**
       This function computes c and s so that
       ( c -s ) ( a )  =  ( 0 )
       s  c     b       ( * )
     */
    void computeUnconventional(const T a, const T b) {
        T d = a * a + b * b;
        c = 0;
        s = 1;
        if(d != 0) {
            // T t = 1 / std::sqrt(d);
            T t = MathHelpers::approx_rsqrt(d);
            s = a * t;
            c = b * t;
        }
    }

    /**
       Fill the R with the entries of this rotation
     */
    template<class MatrixType>
    void fill(const MatrixType& R) const {
        MatrixType& A = const_cast<MatrixType&>(R);
        A = MatrixType(1.0);
        A[rowi][rowi] = c;
        A[rowk][rowi] = s;
        A[rowi][rowk] = -s;
        A[rowk][rowk] = c;
    }

    /**
       This function does something like
       c -s  0
       ( s  c  0 ) A -> A
       0  0  1
       It only affects row i and row k of A.
     */
    template<class MatrixType>
    void rowRotation(MatrixType& A) const {
        for(Int j = 0; j < A.length(); j++) {
            T tau1 = A[j][rowi];
            T tau2 = A[j][rowk];
            A[j][rowi] = c * tau1 - s * tau2;
            A[j][rowk] = s * tau1 + c * tau2;
        }
    }

    /**
       This function does something like
       c  s  0
       A ( -s  c  0 )  -> A
       0  0  1
       It only affects column i and column k of A.
     */
    template<class MatrixType>
    void columnRotation(MatrixType& A) const {
        for(Int j = 0; j < A.length(); j++) {
            T tau1 = A[rowi][j];
            T tau2 = A[rowk][j];
            A[rowi][j] = c * tau1 - s * tau2;
            A[rowk][j] = s * tau1 + c * tau2;
        }
    }

    /**
       Multiply givens must be for same row and column
     **/
    void operator*=(const GivensRotation<T>& A) {
        T new_c = c * A.c - s * A.s;
        T new_s = s * A.c + c * A.s;
        c = new_c;
        s = new_s;
    }

    /**
       Multiply givens must be for same row and column
     **/
    GivensRotation<T> operator*(const GivensRotation<T>& A) const {
        GivensRotation<T> r(*this);
        r *= A;
        return r;
    }
};

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
/**
   \brief zero chasing the 3X3 matrix to bidiagonal form
   original form of H:
   x x 0
   x x x
   0 0 x
   after zero chase:
   x x 0
   0 x x
   0 0 x
 */
template<class T>
void zeroChase(Mat3x3<T>& H, Mat3x3<T>& U, Mat3x3<T>& V) {
    /**
       Reduce H to of form
       x x +
       0 x x
       0 0 x
     */
    GivensRotation<T> r1(H[0][0], H[0][1], 0, 1);
    /**
       Reduce H to of form
       x x 0
       0 x x
       0 + x
       Can calculate r2 without multiplying by r1 since both entries are in first two
       rows thus no need to divide by std::sqrt(a^2+b^2)
     */
    GivensRotation<T> r2(1, 2);
    if(H[0][1] != 0) {
        r2.compute(H[0][0] * H[1][0] + H[0][1] * H[1][1], H[0][0] * H[2][0] + H[0][1] * H[2][1]);
    } else {
        r2.compute(H[1][0], H[2][0]);
    }

    r1.rowRotation(H);

    /* GivensRotation<T> r2(H[0][1], H[0][2], 1, 2); */
    r2.columnRotation(H);
    r2.columnRotation(V);

    /**
       Reduce H to of form
       x x 0
       0 x x
       0 0 x
     */
    GivensRotation<T> r3(H[1][1], H[1][2], 1, 2);
    r3.rowRotation(H);

    // Save this till end for better cache coherency
    // r1.rowRotation(u_transpose);
    // r3.rowRotation(u_transpose);
    r1.columnRotation(U);
    r3.columnRotation(U);
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
/**
   \brief make a 3X3 matrix to upper bidiagonal form
   original form of H:   x x x
   x x x
   x x x
   after zero chase:
   x x 0
   0 x x
   0 0 x
 */
template<class T>
void makeUpperBidiag(Mat3x3<T>& H, Mat3x3<T>& U, Mat3x3<T>& V) {
    U = Mat3x3<T>(1.0);
    V = Mat3x3<T>(1.0);

    /**
       Reduce H to of form
       x x x
       x x x
       0 x x
     */

    GivensRotation<T> r(H[0][1], H[0][2], 1, 2);
    r.rowRotation(H);
    // r.rowRotation(u_transpose);
    r.columnRotation(U);
    // zeroChase(H, u_transpose, V);
    zeroChase(H, U, V);
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
/**
   \brief make a 3X3 matrix to lambda shape
   original form of H:   x x x
 *                     x x x
 *                     x x x
   after :
 *                     x 0 0
 *                     x x 0
 *                     x 0 x
 */
template<class T>
void makeLambdaShape(Mat3x3<T>& H, Mat3x3<T>& U, Mat3x3<T>& V) {
    U = Mat3x3<T>(1.0);
    V = Mat3x3<T>(1.0);

    /**
       Reduce H to of form
     *                    x x 0
     *                    x x x
     *                    x x x
     */

    GivensRotation<T> r1(H[1][0], H[2][0], 1, 2);
    r1.columnRotation(H);
    r1.columnRotation(V);

    /**
       Reduce H to of form
     *                    x x 0
     *                    x x 0
     *                    x x x
     */

    r1.computeUnconventional(H[2][1], H(2, 2));
    r1.rowRotation(H);
    r1.columnRotation(U);

    /**
       Reduce H to of form
     *                    x x 0
     *                    x x 0
     *                    x 0 x
     */

    GivensRotation<T> r2(H[0][2], H[1][2], 0, 1);
    r2.columnRotation(H);
    r2.columnRotation(V);

    /**
       Reduce H to of form
     *                    x 0 0
     *                    x x 0
     *                    x 0 x
     */
    r2.computeUnconventional(H[1][0], H[1][1]);
    r2.rowRotation(H);
    r2.columnRotation(U);
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
/**
   \brief 2x2 polar decomposition.
   \param[in] A matrix.
   \param[out] R Robustly a rotation matrix in givens form
   \param[out] S_Sym Symmetric. Whole matrix is stored

   Whole matrix S is stored since its faster to calculate due to simd vectorization
   Polar guarantees negative sign is on the small magnitude singular value.
   S is guaranteed to be the closest one to identity.
   R is guaranteed to be the closest rotation to A.
 */
template<class T>
void polarDecomposition(const Mat2x2<T>& A, GivensRotation<T>& R, const Mat2x2<T>& S_Sym) {
    Vec2<T> x(A[0][0] + A[1][1], A[0][1] - A[1][0]);
    T       denominator = glm::length(x);
    R.c = (T)1;
    R.s = (T)0;
    if(denominator != 0) {
        /*
           No need to use a tolerance here because x[0] and x[1] always have
           smaller magnitude then denominator, therefore overflow never happens.
         */
        R.c = x[0] / denominator;
        R.s = -x[1] / denominator;
    }
    Mat2x2<T>& S = const_cast<Mat2x2<T>&>(S_Sym);
    S = A;
    R.rowRotation(S);
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
/**
   \brief 2x2 polar decomposition.
   \param[in] A matrix.
   \param[out] R Robustly a rotation matrix.
   \param[out] S_Sym Symmetric. Whole matrix is stored

   Whole matrix S is stored since its faster to calculate due to simd vectorization
   Polar guarantees negative sign is on the small magnitude singular value.
   S is guaranteed to be the closest one to identity.
   R is guaranteed to be the closest rotation to A.
 */
template<class T>
void polarDecomposition(const Mat2x2<T>& A, const Mat2x2<T>& R, const Mat2x2<T>& S_Sym) {
    GivensRotation<T> r(0, 1);
    polarDecomposition(A, r, S_Sym);
    r.fill(R);
}

template<class T>
std::pair<Mat2x2<T>, Mat2x2<T>> polarDecomposition(const Mat2x2<T>& A) {
    Mat2x2<T> R, S_Sym;
    polarDecomposition(A, R, S_Sym);
    return std::make_pair(R, S_Sym);
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
/**
   \brief 2x2 SVD (singular value decomposition) A=USV'
   \param[in] A Input matrix.
   \param[out] U Robustly a rotation matrix in Givens form
   \param[out] Sigma StdVT of singular values sorted with decreasing magnitude. The second one can be negative.
   \param[out] V Robustly a rotation matrix in Givens form
 */
template<class T>
void svd(const Mat2x2<T>& A, GivensRotation<T>& U, Vec2<T>& sigma, GivensRotation<T>& V, T tol = T(64.0)* std::numeric_limits<T>::epsilon()) {
    (void)tol;
    Mat2x2<T> S_Sym;
    polarDecomposition(A, U, S_Sym);
    T cosine, sine;
    T x = S_Sym[0][0];
    T y = S_Sym[1][0];
    T z = S_Sym[1][1];
    if(std::abs(y) < T(1e-20)) {
        // S is already diagonal
        cosine   = T(1.0);
        sine     = T(0);
        sigma[0] = x;
        sigma[1] = z;
    } else {
        T tau = T(0.5) * (x - z);
        T w   = std::sqrt(tau * tau + y * y);
        // w > y > 0
        T t;
        if(tau > 0) {
            // tau + w > w > y > 0 ==> division is safe
            t = y / (tau + w);
        } else {
            // tau - w < -w < -y < 0 ==> division is safe
            t = y / (tau - w);
        }
        cosine = T(1) / std::sqrt(t * t + T(1));
        sine   = -t * cosine;
        /*
           V = [cosine -sine; sine cosine]
           Sigma = V'SV. Only compute the diagonals for efficiency.
           Also utilize symmetry of S and don't form V yet.
         */
        T c2  = cosine * cosine;
        T csy = T(2.0) * cosine * sine * y;
        T s2  = sine * sine;
        sigma[0] = c2 * x - csy + s2 * z;
        sigma[1] = s2 * x + csy + c2 * z;
    }

    // Sorting
    // Polar already guarantees negative sign is on the small magnitude singular value.
    if(sigma[0] < sigma[1]) {
        std::swap(sigma[0], sigma[1]);
        V.c = -sine;
        V.s = cosine;
    } else {
        V.c = cosine;
        V.s = sine;
    }
    U *= V;
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
/**
   \brief 2x2 SVD (singular value decomposition) A=USV'
   \param[in] A Input matrix.
   \param[out] U Robustly a rotation matrix.
   \param[out] Sigma StdVT of singular values sorted with decreasing magnitude. The second one can be negative.
   \param[out] V Robustly a rotation matrix.
 */
template<class T>
void svd(const Mat2x2<T>& A, const Mat2x2<T>& U, Vec2<T>& Sigma, const Mat2x2<T>& V, T tol = T(64.0)* std::numeric_limits<T>::epsilon()) {
    (void)tol;
    GivensRotation<T> gv(0, 1);
    GivensRotation<T> gu(0, 1);
    svd(A, gu, Sigma, gv);

    gu.fill(U);
    gv.fill(V);
}

template<class T>
std::tuple<Mat2x2<T>, Vec2<T>, Mat2x2<T>> svd(const Mat2x2<T>& A) {
    Mat2x2<T> U, V;
    Vec2<T>   Sigma;
    svd(A, U, Sigma, V);
    return std::make_tuple(U, Sigma, V);
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
/**
   \brief compute wilkinsonShift of the block
   a1     b1
   b1     a2
   based on the wilkinsonShift formula
   mu = c + d - sign (d) \ std::sqrt (d*d + b*b), where d = (a-c)/2

 */
template<class T>
T wilkinsonShift(const T a1, const T b1, const T a2) {
    T d  = T(0.5) * (a1 - a2);
    T bs = b1 * b1;

    T mu = a2 - copysign(bs / (std::abs(d) + std::sqrt(d * d + bs)), d);
    // T mu = a2 - bs / ( d + sign_d*sqrt (d*d + bs));
    return mu;
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
/**
   \brief Helper function of 3X3 SVD for processing 2X2 SVD
 */
template<Int t, class T>
void process(Mat3x3<T>& B, Mat3x3<T>& U, Vec3<T>& sigma, Mat3x3<T>& V) {
    Int               other = (t == 1) ? 0 : 2;
    GivensRotation<T> u(0, 1);
    GivensRotation<T> v(0, 1);
    sigma[other] = B[other][other];

    Mat2x2<T> A;
    Vec2<T>   s;
    for(Int j = 0; j < 2; ++j) {
        s[j] = sigma[j + t];
        for(Int i = 0; i < 2; ++i) {
            A[i][j] = B[i + t][j + t];
        }
    }
    svd(A, u, s, v);
    for(Int j = 0; j < 2; ++j) {
        sigma[j + t] = s[j];
        for(Int i = 0; i < 2; ++i) {
            B[i + t][j + t] = A[i][j];
        }
    }

    u.rowi += t;
    u.rowk += t;
    v.rowi += t;
    v.rowk += t;
    u.columnRotation(U);
    v.columnRotation(V);
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
/**
   \brief Helper function of 3X3 SVD for flipping signs due to flipping signs of sigma
 */
template<class T>
void flipSign(Int i, Mat3x3<T>& U, Vec3<T>& sigma) {
    sigma[i] = -sigma[i];
    U[i]     = -U[i];
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
/**
   \brief Helper function of 3X3 SVD for sorting singular values
 */
template<class T>
void sort0(Mat3x3<T>& U, Vec3<T>& sigma, Mat3x3<T>& V) {
    // Case: sigma[0] > |sigma[1]| >= |sigma[2]|
    if(std::abs(sigma[1]) >= std::abs(sigma[2])) {
        if(sigma[1] < 0) {
            flipSign(1, U, sigma);
            flipSign(2, U, sigma);
        }
        return;
    }

    //fix sign of sigma for both cases
    if(sigma[2] < 0) {
        flipSign(1, U, sigma);
        flipSign(2, U, sigma);
    }

    //swap sigma[1] and sigma[2] for both cases
    std::swap(sigma[1], sigma[2]);
    std::swap(U[1],     U[2]);
    std::swap(V[1],     V[2]);

    // Case: |sigma[2]| >= sigma[0] > |simga[1]|
    if(sigma[1] > sigma[0]) {
        std::swap(sigma[0], sigma[1]);
        std::swap(U[0],     U[1]);
        std::swap(V[0],     V[1]);
    }
    // Case: sigma[0] >= |sigma[2]| > |simga[1]|
    else {
        U[2] = -U[2];
        V[2] = -V[2];
    }
}

/**
   \brief Helper function of 3X3 SVD for sorting singular values
 */
template<class T>
void sort1(Mat3x3<T>& U, Vec3<T>& sigma, Mat3x3<T>& V) {
    // Case: |sigma[0]| >= sigma[1] > |sigma[2]|
    if(std::abs(sigma[0]) >= sigma[1]) {
        if(sigma[0] < 0) {
            flipSign(0, U, sigma);
            flipSign(2, U, sigma);
        }
        return;
    }

    //swap sigma[0] and sigma[1] for both cases
    std::swap(sigma[0], sigma[1]);
    std::swap(U[0],     U[1]);
    std::swap(V[0],     V[1]);

    // Case: sigma[1] > |sigma[2]| >= |sigma[0]|
    if(std::abs(sigma[1]) < std::abs(sigma[2])) {
        std::swap(sigma[1], sigma[2]);
        std::swap(U[1],     U[2]);
        std::swap(V[1],     V[2]);
    }
    // Case: sigma[1] >= |sigma[0]| > |sigma[2]|
    else {
        U[1] = -U[1];
        V[1] = -V[1];
    }

    // fix sign for both cases
    if(sigma[1] < 0) {
        flipSign(1, U, sigma);
        flipSign(2, U, sigma);
    }
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
/**
   \brief 3X3 SVD (singular value decomposition) A=USV'
   \param[in] A Input matrix.
   \param[out] U is a rotation matrix.
   \param[out] sigma Diagonal matrix, sorted with decreasing magnitude. The third one can be negative.
   \param[out] V is a rotation matrix.
 */
template<class T>
Int svd(const Mat3x3<T>& A, Mat3x3<T>& U, Vec3<T>& sigma, Mat3x3<T>& V, T tol = T(128.0)* std::numeric_limits<T>::epsilon()) {
    Mat3x3<T> B = A;
    U = Mat3x3<T>(1.0);
    V = Mat3x3<T>(1.0);

    makeUpperBidiag(B, U, V);

    Int               count = 0;
    T                 mu    = (T)0;
    GivensRotation<T> r(0, 1);

    T alpha_1 = B[0][0];
    T beta_1  = B[1][0];
    T alpha_2 = B[1][1];
    T alpha_3 = B[2][2];
    T beta_2  = B[2][1];
    T gamma_1 = alpha_1 * beta_1;
    T gamma_2 = alpha_2 * beta_2;
    tol *= MathHelpers::max(T(0.5) * std::sqrt(alpha_1 * alpha_1 + alpha_2 * alpha_2 + alpha_3 * alpha_3 + beta_1 * beta_1 + beta_2 * beta_2), T(1.0));

    /**
       Do implicit shift QR until A^T A is block diagonal
     */

    while(std::abs(beta_2) > tol && std::abs(beta_1) > tol && std::abs(alpha_1) > tol && std::abs(alpha_2) > tol && std::abs(alpha_3) > tol) {
        mu = wilkinsonShift(alpha_2 * alpha_2 + beta_1 * beta_1, gamma_2, alpha_3 * alpha_3 + beta_2 * beta_2);

        r.compute(alpha_1 * alpha_1 - mu, gamma_1);
        r.columnRotation(B);

        r.columnRotation(V);
        zeroChase(B, U, V);

        alpha_1 = B[0][0];
        beta_1  = B[1][0];
        alpha_2 = B[1][1];
        alpha_3 = B[2][2];
        beta_2  = B[2][1];
        gamma_1 = alpha_1 * beta_1;
        gamma_2 = alpha_2 * beta_2;
        count++;
    }

    /**
       Handle the cases of one of the alphas and betas being 0
       Sorted by ease of handling and then frequency
       of occurrence

       If B is of form
       x x 0
       0 x 0
       0 0 x
     */
    if(std::abs(beta_2) <= tol) {
        process<0>(B, U, sigma, V);
        sort0(U, sigma, V);
    }
    /**
       If B is of form
       x 0 0
       0 x x
       0 0 x
     */
    else if(std::abs(beta_1) <= tol) {
        process<1>(B, U, sigma, V);
        sort1(U, sigma, V);
    }
    /**
       If B is of form
       x x 0
       0 0 x
       0 0 x
     */
    else if(std::abs(alpha_2) <= tol) {
        /**
           Reduce B to
           x x 0
           0 0 0
           0 0 x
         */
        GivensRotation<T> r1(1, 2);
        r1.computeUnconventional(B[2][1], B[2][2]);
        r1.rowRotation(B);
        r1.columnRotation(U);

        process<0>(B, U, sigma, V);
        sort0(U, sigma, V);
    }
    /**
       If B is of form
       x x 0
       0 x x
       0 0 0
     */
    else if(std::abs(alpha_3) <= tol) {
        /**
           Reduce B to
           x x +
           0 x 0
           0 0 0
         */
        GivensRotation<T> r1(1, 2);
        r1.compute(B[1][1], B[2][1]);
        r1.columnRotation(B);
        r1.columnRotation(V);
        /**
           Reduce B to
           x x 0
         + x 0
           0 0 0
         */
        GivensRotation<T> r2(0, 2);
        r2.compute(B[0][0], B[2][0]);
        r2.columnRotation(B);
        r2.columnRotation(V);

        process<0>(B, U, sigma, V);
        sort0(U, sigma, V);
    }
    /**
       If B is of form
       0 x 0
       0 x x
       0 0 x
     */
    else if(std::abs(alpha_1) <= tol) {
        /**
           Reduce B to
           0 0 +
           0 x x
           0 0 x
         */
        GivensRotation<T> r1(0, 1);
        r1.computeUnconventional(B[1][0], B[1][1]);
        r1.rowRotation(B);
        r1.columnRotation(U);

        /**
           Reduce B to
           0 0 0
           0 x x
           0 + x
         */
        GivensRotation<T> r2(0, 2);
        r2.computeUnconventional(B[2][0], B[2][2]);
        r2.rowRotation(B);
        r2.columnRotation(U);

        process<1>(B, U, sigma, V);
        sort1(U, sigma, V);
    }

    return count;
}

template<class T>
std::tuple<Mat3x3<T>, Vec3<T>, Mat3x3<T>> svd(const Mat3x3<T>& A) {
    Mat3x3<T> U, V;
    Vec3<T>   Sigma;
    svd(A, U, Sigma, V);
    return std::make_tuple(U, Sigma, V);
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
/**
   \brief 3X3 polar decomposition.
   \param[in] A matrix.
   \param[out] R Robustly a rotation matrix.
   \param[out] S_Sym Symmetric. Whole matrix is stored

   Whole matrix S is stored
   Polar guarantees negative sign is on the small magnitude singular value.
   S is guaranteed to be the closest one to identity.
   R is guaranteed to be the closest rotation to A.
 */
template<class T>
void polarDecomposition(const Mat3x3<T>& A, Mat3x3<T>& R, Mat3x3<T>& S_Sym) {
    const auto [U, sigma, V] = svd(A);
    const auto Vt = glm::transpose(V);
    R     = U * Vt;
    S_Sym = V * Mat3x3<T>(sigma[0], 0, 0,
                          0, sigma[1], 0,
                          0, 0, sigma[2]) * Vt;
}

template<class T>
std::pair<Mat3x3<T>, Mat3x3<T>> polarDecomposition(const Mat3x3<T>& A) {
    Mat3x3<T> R, S_Sym;
    polarDecomposition(A, R, S_Sym);
    return std::make_pair(R, S_Sym);
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
template<Int M, Int N, class T>
void inplaceGivensR(MatMxN<M, N, T>& A) {
    for(Int j = 0; j < N; j++) {
        for(Int i = M - 1; i > j; i--) {
            GivensRotation<T> r(A[j][i - 1], A[j][i], i - 1, i);
            r.rowRotation(A);
        }
    }
}

template<Int M, Int N, Int L, class T>
void simultaneousGivensQR(MatMxN<M, N, T>& A, MatMxN<M, L, T>& Q) {
    for(Int j = 0; j < N; j++) {
        for(Int i = M - 1; i > j; i--) {
            GivensRotation<T> r(A[j][i - 1], A[j][i], i - 1, i);
            r.rowRotation(A);
            r.rowRotation(Q);
        }
    }
    // A <- Q.transpose() * A = R
    // Q <- Q.transpose() * Q
    // thin or thick A, M not yet tested
}

template<Int M, Int N, class T>
void inplaceGivensQR(MatMxN<M, N, T>& A, MatMxN<M, M, T>& Q) {
    Q = MatMxN<M, M, T>(1);
    simultaneousGivensQR(A, Q);
    Q = glm::transpose(Q);
}

template<Int M, Int N, class T>
MatMxN<M, M, T> inplaceGivensQR(MatMxN<M, N, T>& A) {
    auto Q = MatMxN<M, M, T>(1);
    simultaneousGivensQR(A, Q);
    return glm::transpose(Q);
}

template<Int M, Int N, class T>
void GivensQR(const MatMxN<M, N, T>& A, MatMxN<M, M, T>& Q, MatMxN<M, N, T>& R) {
    R = A;
    inplaceGivensQR(R, Q);
}

template<Int M, Int N, class T>
std::pair<MatMxN<M, M, T>, MatMxN<M, N, T>> GivensQR(const MatMxN<M, N, T>& A) {
    MatMxN<M, M, T> Q;
    MatMxN<M, N, T> R;
    GivensQR(A, Q, R);
    return std::make_pair(Q, R);
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#define __BNN_INSTANTIATE_QRSVD_FUNCS(dim, type)                                                                     \
    template std::tuple<MatXxX<dim, type>, VecX<dim, type>, MatXxX<dim, type>> svd<type>(const MatXxX<dim, type>&A); \
    template std::pair<MatXxX<dim, type>, MatXxX<dim, type>> polarDecomposition<type>(const MatXxX<dim, type>&A);    \
    template MatMxN<dim, dim, type> inplaceGivensQR<dim, dim, type>(MatMxN<dim, dim, type>&);                        \
    template std::pair<MatMxN<dim, dim, type>, MatMxN<dim, dim, type>> GivensQR<dim, dim, type>(const MatMxN<dim, dim, type>&);

__BNN_INSTANTIATE_QRSVD_FUNCS(2, float)
__BNN_INSTANTIATE_QRSVD_FUNCS(2, double)
__BNN_INSTANTIATE_QRSVD_FUNCS(3, float)
__BNN_INSTANTIATE_QRSVD_FUNCS(3, double)
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
} // end namespace NTCodeBase::QRSVD
