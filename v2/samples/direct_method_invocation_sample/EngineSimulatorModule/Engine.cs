namespace EngineSimulatorModule
{
    using System;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Runtime.Loader;
    using System.Security.Cryptography.X509Certificates;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Devices.Client;
    using Microsoft.Azure.Devices.Client.Transport.Mqtt;

    // This class contains a simple form of a simulated engine.
    // It has 2 methods -
    // 1. GetCurrentRpm gets the current engine RPM. It also sets the next RPM value (for the simulation)
    // 2. Reset method resets the RPM to its initial value.
    class Engine
    {
        const int DeltaRange = 5;
        const double InitVal = 10;
        static Random rand = new Random();        
        double currentRpm = InitVal;
        
        public double GetCurrentRpm()
        {
            double rpm = this.currentRpm;
            this.currentRpm = GetNextRpm(this.currentRpm);
            return rpm;
        }

        public void Reset() => this.currentRpm = InitVal;

        static double GetNextRpm(double currrntRpm) => currrntRpm + rand.Next(0, DeltaRange) + rand.NextDouble();
    }
}