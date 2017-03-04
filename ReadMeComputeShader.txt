/************************************************************************************************
------------------------------------------------------------------------------------------------
On the use of vec4:
GLSL assigns every value (int, bool, float, char, etc.) a 16-byte value.  This demo program is 
in 2D only, so I only need a vec2 for all the points that I need, but simply adding 2 floats of 
padding to the corresponding vec2 structures on the CPU side didn't seem to line them up.  So I
decided to use a vec4 for all my points.  It made my life easier.

------------------------------------------------------------------------------------------------
On specifying the work item count:
Ex: layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

This demo program goes through a large array of particles, but a whole bunch of shader instances 
are running at one time.  Compute shaders introduce order by keeping track of a 3D work item 
count.  For each work group, the number of which is specified on the CPU side of the program 
when the compute shader is summoned, all the work items will be called.  

For example, supposed there are 10,000 particles in the array.  The compute shader is kicked 
into gear by 
glDispatchCompute(numWorkGroupsX, numWorkGroupsY, and numWorkGroupsZ).  
Since compute shaders are often used to compute for 3D gaming or rendering environments, this 
made sense to the creators of the OpenGL compute shader standard.  

Now suppose that I want to make 256 particles on each compute thread.  Then 
10,000 / 256 = ~39 work groups.  This means (??you sure??) that there will be 39 parallel 
threads of this shader and the GPU will schedule them whenever it can.  At the time of this 
comment (9-3-2016), there doesn't seem to be an optimal number of work groups 
(http://gamedev.stackexchange.com/questions/66198/optimal-number-of-work-groups-for-compute-shaders).  

When I specify the layout, I must use this same number (256).  To get 10,000 work items 
(individual calls to the compute shader's main()), then I must do this:
CPU code: glDispatchCompute(39, 1, 1);
GPU code: layout (local_size_x = 256, local_size_y = 1, local_size_z = 1);
Equivalent GPU code: layout (local_size_x = 256);

The global work item number for any given call to the compute shader's main() is 
"local work group number * local work item number".  
Ex: Suppose the GPU is working on work group 24 (out of 39), and this thread is on invocation 
159 (out of 256).  Then the global work item number is 24 * 159 = 3816.

From the OpenGL ARB (Architecture Review Board): 
    https://www.opengl.org/registry/specs/ARB/compute_shader.txt
        The built-in variable <gl_LocalInvocationID> is a compute-shader input
    variable containing the 3-dimensional index of the local work group
    within the global work group that the current invocation is executing in.
    The possible values for this variable range across the local work group
    size, i.e. (0,0,0) to (gl_WorkGroupSize.x - 1, gl_WorkGroupSize.y - 1,
    gl_WorkGroupSize.z - 1).

The "invocation ID" (the global work item number) starts at 0, so it is therefore suitable as an 
index.  Now I can use the work item number as an index into the particle array.  

Now why "x"?  Because I decided to use the X axis in the compute shader.  I could have done it 
with Y or Z or did some kind of computation to work out an index into the particle array, but it 
was easier to just use one axis of the compute shader for a 1D array.

I could call "*.y" or "*.z", but that wouldn't be helpful size the layout specifies those as 1.  
According to the equation given earlier for the work item number and the "local size" of Y and Z 
being 1, the global work item on Y and Z will always be "work group * 1".  That means only 40 
work items in this example.  Not helpful when I have 10,000 particles.  

Anyway, to get the global work item number for an individual call to the compute shader's 
main(), I call this:
uint i = gl_GlobalInvocationID.x;

Problem: This example will only summon 39 * 256 = 9,984 work items.  This is less than the size 
of the particle array, so if I use the work item number as the index, some particles will never 
be accessed.  
Solution: On the CPU it is good practice to instead do this:
(10,000 / 256) + 1 = ~40.  And 40 * 256 = 10,240 work items.  This is greater than the array 
size, so now I need to ignore work item numbers that exceed the max index.

Then I use a uniform to tell me how many particles are actually in the array.  I compare the 
work item number against it, and if it exceeds the total number of particles, then the shader 
doesn't do anything.  Will there be some wasted calls?  Sure, but the GPU keeps its cores busy 
with other calls, so it isn't much of a performance loss.

Sample code:

uniform uint uMaxParticleCount;
void main()
{
    uint index = gl_GlobalInvocationID.x;
    if (index < uMaxParticleCount)
    {
        // do the thing
    }
    else
    {
        // nothing to do, so this GPU core immediately returns and moves on to the next job
    }
}

------------------------------------------------------------------------------------------------
On accessing and changing items in a buffer:
I specify a single buffer in this manner:

layout (binding = 0) buffer ParticleBuffer {
    Particle AllParticles[];
};

This is a special buffer called a Uniform Buffer Object (UBO).  In this demo, the uniform buffer 
object contains a large, 1D array of data formatted after the Particle structure.  I could have 
told this shader that the data is formatted differently and it would have believed me, but then 
the bytes wouldn't line up and it would be useless.

Anyway.  There are no pointers or references in GLSL.  Everything is copy only, which is a good 
thing during parallel workloads because it means that you have to be very deliberate to intrude 
on another thread's working data.

To change a particle:
Particle p = AllParticles[someIndex];
...change particle data...
AllParticles[theSameIndex] = p;

Where "someIndex" is the global work item assigned to this particular invocation of the compute 
shader's main().  If I access any other index, then I am running over another thread's work.  

Multithread responsibly.


************************************************************************************************/