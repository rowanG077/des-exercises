/*
 * generated by Xtext 2.22.0
 */
package ev3.behaviours;


/**
 * Initialization support for running Xtext languages without Equinox extension registry.
 */
public class DSLStandaloneSetup extends DSLStandaloneSetupGenerated {

	public static void doSetup() {
		new DSLStandaloneSetup().createInjectorAndDoEMFRegistration();
	}
}
