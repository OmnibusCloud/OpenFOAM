{
    fvScalarMatrix hEqn
    (
        fvm::ddt(betav*rho, h)
      - thermo.heatDiffusion(betav, h)
      ==
        fvOptions(rho, h)
    );

    if (meshFluxCorr)
    {
        surfaceScalarField phihMesh
        (
            fvc::interpolate(betav*rho*h)*mesh.phi()
        );

       hEqn -= fvc::div(phihMesh);
    }

    hEqn.relax();

    fvOptions.constrain(hEqn);

    hEqn.solve();   //mesh.solver(h.select(finalIter)));

    fvOptions.correct(h);

    thermo.correct();

    Info<< "Min/max T:" << min(thermo.T()).value() << ' '
        << max(thermo.T()).value() << endl;

    radiation->correct();
}
