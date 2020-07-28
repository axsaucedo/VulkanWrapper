#version 450

// Colour output from subpass 1
layout(input_attachment_index = 0, binding = 0) uniform subpassInput inputColour; 
// Depth output from subpass 1
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth; 

layout(location = 0) out vec4 colour;

void main() {
	// colour = subpassLoad(inputColour).rgba;
	int xHalf = 1920 / 2;
	if(gl_FragCoord.x > xHalf) {
		float lowerBound = 0.99;
		float upperBound = 1;

		float depth = subpassLoad(inputDepth).r;
		float depthColourScaled = 1.0f - ((depth - lowerBound) / (upperBound - lowerBound));

		colour = vec4(subpassLoad(inputColour).rgb * depthColourScaled, 1.0f);
	} else {
		colour = subpassLoad(inputColour).rgba;
	}
}
