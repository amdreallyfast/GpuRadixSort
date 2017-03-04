#version 440

// Note: The vec2's are in window space (both X and Y on the range [-1,+1])
// Also Note: The vec2s are provided as vec4s on the CPU side and specified as such in the 
// vertex array attributes, but it is ok to only take them as a vec2, as I am doing for this 
// 2D demo.
layout (location = 0) in vec4 pos;  
layout (location = 1) in vec4 vel;  
layout (location = 2) in vec4 netForce;
layout (location = 3) in int collisionCountThisFrame;
layout (location = 4) in float mass;
layout (location = 5) in float radiusOfInfluence;
layout (location = 6) in uint indexOfNodeThatItIsOccupying;
layout (location = 7) in int isActive;

// must have the same name as its corresponding "in" item in the frag shader
smooth out vec4 particleColor;

void main()
{
    if (isActive == 0)
    {
        // invisible (alpha = 0), but "fully transparent" does not mean "no color", it merely 
        // means that the color of this thing will be added to the thing behind it (see Z 
        // adjustment later)
        particleColor = vec4(0.0f, 1.0f, 0.0f, 0.0f);
        gl_Position = vec4(pos.xy, -0.6f, 1.0f);
    }
    else
    {
        float red = 0.0f;       // high velocity
        float green = 0.0f;     // medium velocity
        float blue = 0.0f;      // low velocity

        float min = 0;
        float mid = 15;
        float max = 30;
    
        // I know that "collision count" is an int, but make a float out of it for the sake of 
        // these calculation
        float value = collisionCountThisFrame;
        //float value = 15;
        if (value < mid)
        {
            // low - medium velocity => linearly blend blue and green
            float fraction = (value - min) / (mid - min);

            // 0 collisions => all blue
            // 14 collisions => all green
            blue = (1.0f - fraction);
            green = fraction;
        }
        else
        {
            // medium - high velocity => linearly blend green and red
            float fraction = (value - mid) / (max - mid);

            // 15 collisions => all green
            // 30+ collisions => all red
            green = (1.0f - fraction);
            red = fraction;
        }

        particleColor = vec4(red, green, blue, 1.0f);
        
//        //                            2147483647
//        //
//        // AllParticles[particleIndex]._collisionCountThisFrame = 8 --> collisionCountThisFrame == 1090519040 --> 0b1000001000000000000000000000000
//        // AllParticles[particleIndex]._collisionCountThisFrame = 7 --> collisionCountThisFrame == 1088421888 --> 0b1000000111000000000000000000000
//        // AllParticles[particleIndex]._collisionCountThisFrame = 1 --> collisionCountThisFrame == 1065353216 --> 0b0111111100000000000000000000000
//        //                            1075838976
//        if (collisionCountThisFrame == 3)
//        //if (mass == 0.1f)
//        //if (indexOfNodeThatItIsOccupying == 13)
//        {
//            particleColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
//        }
//        else
//        {
//            particleColor = vec4(0.0f, 1.0f, 1.0f, 1.0f);
//        }

        // active => opaque (alpha = 1) white
        //particleColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
        gl_Position = vec4(pos.xy, -0.7f, 1.0f);
    }
}

