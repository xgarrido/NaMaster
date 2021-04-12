import pytest
import numpy as np
import healpy as hp
import pymaster as nmt
import warnings
import sys


class CovarTester(object):
    def __init__(self):
        # This is to avoid showing an ugly warning
        # that has nothing to do with pymaster
        if (sys.version_info > (3, 1)):
            warnings.simplefilter("ignore", ResourceWarning)

        self.nside = 64
        self.nlb = 16
        self.npix = hp.nside2npix(self.nside)
        msk = hp.read_map("test/benchmarks/msk.fits", verbose=False)
        mps = np.array(hp.read_map("test/benchmarks/mps.fits",
                                   verbose=False, field=[0, 1, 2]))
        self.b = nmt.NmtBin.from_nside_linear(self.nside, self.nlb)
        self.f0 = nmt.NmtField(msk, [mps[0]])
        self.f2 = nmt.NmtField(msk, [mps[1], mps[2]])
        self.f0_half = nmt.NmtField(msk[:self.npix//4],
                                    [mps[0, :self.npix//4]])  # Half nside
        self.w = nmt.NmtWorkspace()
        self.w.read_from("test/benchmarks/bm_nc_np_w00.fits")
        self.w02 = nmt.NmtWorkspace()
        self.w02.read_from("test/benchmarks/bm_nc_np_w02.fits")
        self.w22 = nmt.NmtWorkspace()
        self.w22.read_from("test/benchmarks/bm_nc_np_w22.fits")

        cls = np.loadtxt("test/benchmarks/cls_lss.txt", unpack=True)
        l, cltt, clee, clbb, clte, nltt, nlee, nlbb, nlte = cls
        self.ll = l[:3*self.nside]
        self.cltt = cltt[:3*self.nside]+nltt[:3*self.nside]
        self.clee = clee[:3*self.nside]+nlee[:3*self.nside]
        self.clbb = clbb[:3*self.nside]+nlbb[:3*self.nside]
        self.clte = clte[:3*self.nside]


CT = CovarTester()


def test_workspace_covar_benchmark():
    def compare_covars(c, cb):
        # Check first and second diagonals
        for k in [0, 1]:
            d = np.diag(c, k=k)
            db = np.diag(cb, k=k)
            assert (np.fabs(d-db) <=
                    np.fmin(np.fabs(d),
                            np.fabs(db))*1E-4).all()

    # Check against a benchmark
    cw = nmt.NmtCovarianceWorkspace()
    cw.compute_coupling_coefficients(CT.f0, CT.f0)

    # [0,0 ; 0,0]
    covar = nmt.gaussian_covariance(cw, 0, 0, 0, 0,
                                    [CT.cltt], [CT.cltt],
                                    [CT.cltt], [CT.cltt],
                                    CT.w)
    covar_bench = np.loadtxt("test/benchmarks/bm_nc_np_cov.txt",
                             unpack=True)
    compare_covars(covar, covar_bench)
    # Check coupled
    covar_rc = nmt.gaussian_covariance(cw, 0, 0, 0, 0,
                                       [CT.cltt], [CT.cltt],
                                       [CT.cltt], [CT.cltt],
                                       CT.w, coupled=True)  # [nl, nl]
    covar_c = np.array([CT.w.decouple_cell([row])[0]
                        for row in covar_rc])  # [nl, nbpw]
    covar = np.array([CT.w.decouple_cell([col])[0]
                      for col in covar_c.T]).T  # [nbpw, nbpw]
    compare_covars(covar, covar_bench)

    # [0,2 ; 0,2]
    covar = nmt.gaussian_covariance(cw, 0, 2, 0, 2,
                                    [CT.cltt],
                                    [CT.clte, 0*CT.clte],
                                    [CT.clte, 0*CT.clte],
                                    [CT.clee, 0*CT.clee,
                                     0*CT.clee, CT.clbb],
                                    CT.w02, wb=CT.w02)
    covar_bench = np.loadtxt("test/benchmarks/bm_nc_np_cov0202.txt")
    compare_covars(covar, covar_bench)
    # [0,0 ; 0,2]
    covar = nmt.gaussian_covariance(cw, 0, 0, 0, 2,
                                    [CT.cltt],
                                    [CT.clte, 0*CT.clte],
                                    [CT.cltt],
                                    [CT.clte, 0*CT.clte],
                                    CT.w, wb=CT.w02)
    covar_bench = np.loadtxt("test/benchmarks/bm_nc_np_cov0002.txt")
    compare_covars(covar, covar_bench)
    # [0,0 ; 2,2]
    covar = nmt.gaussian_covariance(cw, 0, 0, 2, 2,
                                    [CT.clte, 0*CT.clte],
                                    [CT.clte, 0*CT.clte],
                                    [CT.clte, 0*CT.clte],
                                    [CT.clte, 0*CT.clte],
                                    CT.w, wb=CT.w22)
    covar_bench = np.loadtxt("test/benchmarks/bm_nc_np_cov0022.txt")
    compare_covars(covar, covar_bench)
    # [2,2 ; 2,2]
    covar = nmt.gaussian_covariance(cw, 2, 2, 2, 2,
                                    [CT.clee, 0*CT.clee,
                                     0*CT.clee, CT.clbb],
                                    [CT.clee, 0*CT.clee,
                                     0*CT.clee, CT.clbb],
                                    [CT.clee, 0*CT.clee,
                                     0*CT.clee, CT.clbb],
                                    [CT.clee, 0*CT.clee,
                                     0*CT.clee, CT.clbb],
                                    CT.w22, wb=CT.w22)
    covar_bench = np.loadtxt("test/benchmarks/bm_nc_np_cov2222.txt")
    compare_covars(covar, covar_bench)


def test_workspace_covar_errors():
    cw = nmt.NmtCovarianceWorkspace()

    with pytest.raises(ValueError):  # Write uninitialized
        cw.write_to("wsp.fits")

    cw.compute_coupling_coefficients(CT.f0, CT.f0)  # All good
    assert cw.wsp.lmax == CT.w.wsp.lmax
    assert cw.wsp.lmax == CT.w.wsp.lmax
    with pytest.raises(RuntimeError):  # Write uninitialized
        cw.write_to("tests/wsp.fits")

    cw.read_from('test/benchmarks/bm_nc_np_cw00.fits')  # Correct reading
    assert cw.wsp.lmax == CT.w.wsp.lmax
    assert cw.wsp.lmax == CT.w.wsp.lmax

    # gaussian_covariance
    with pytest.raises(ValueError):  # Wrong input cl size
        nmt.gaussian_covariance(cw, 0, 0, 0, 0,
                                [CT.cltt], [CT.cltt],
                                [CT.cltt], [CT.cltt[:15]],
                                CT.w)
    with pytest.raises(ValueError):  # Wrong input cl shapes
        nmt.gaussian_covariance(cw, 0, 0, 0, 0,
                                [CT.cltt], [CT.cltt],
                                [CT.cltt], [CT.cltt, CT.cltt],
                                CT.w)
    with pytest.raises(ValueError):  # Wrong input spins
        nmt.gaussian_covariance(cw, 0, 2, 0, 0,
                                [CT.cltt], [CT.cltt],
                                [CT.cltt], [CT.cltt, CT.cltt],
                                CT.w)

    with pytest.raises(RuntimeError):  # Incorrect reading
        cw.read_from('none')

    with pytest.raises(ValueError):  # Incompatible resolutions
        cw.compute_coupling_coefficients(CT.f0, CT.f0_half)
