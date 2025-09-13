#ifndef ibl_common_hlsli
#define ibl_common_hlsli    


// https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-20-gpu-based-importance-sampling
// https://cgg.mff.cuni.cz/~jaroslav/papers/2007-sketch-fis/Final_sap_0073.pdf
float computeLod(float pdf, int numSamples, int width)
{
    // // note that 0.5 * log2 is equivalent to log4
    return max(0.5 * log2( 6.0 * float(width) * float(width) / (float(numSamples) * pdf)), 0.0);
}



#endif /* ibl_common_hlsli */