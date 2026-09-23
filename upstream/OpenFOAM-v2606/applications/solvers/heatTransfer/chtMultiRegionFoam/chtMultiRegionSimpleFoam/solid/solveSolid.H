{
    for (int nonOrth=0; nonOrth<=nNonOrthCorr; ++nonOrth)
    {
        fvScalarMatrix hEqn
        (
            -thermo.heatDiffusion(betav, h)
          ==
            fvOptions(rho, h)
        );

        hEqn.relax();

        fvOptions.constrain(hEqn);

        if (coupled)
        {
            fvMatrixAssemblyPtr->addFvMatrix(hEqn);
        }
        else
        {
            Info<< "\nSolving for solid region "
                << solidRegions[i].name() << endl;

            hEqn.solve();
            fvOptions.correct(h);
            thermo.correct();

            Info<< "Min/max T:" << min(thermo.T()).value() << ' '
                << max(thermo.T()).value() << endl;
        }
    }
}
