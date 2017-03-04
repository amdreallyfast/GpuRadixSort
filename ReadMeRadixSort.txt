/*------------------------------------------------------------------------------------------------
Description:
    Radix sorting is a "positional" sorting algorithm, not a non-comparative sorting algorithm.  
    Instead of running through an array and comparing the current index's value to nearby 
    values, it instead performs multiple passes through the values' base digits or bits.

    After each pass, all "like items" (that is, items with the same value in particular digit or 
    bit) will be gathered together.  
    Ex base-10 counting sytem: 
        The result is that all entries with a 0 in the 10s digit will come first, all entries 
        with a 1 in the 10s digit will come second, with a 2 in the 10s digit 3rd, ..., with a 9 
        in the 10s digit last.
    Ex base-2 counting system: 
        The result is that all entries with a 0 in the 1st bit (not the 0th bit; bit positions 
        start at 0, which was dealt with for the 1st pass) will come first, and all entries with 
        a 1 in the 1st bit will come last.

    CRUCIAL: After each pass, all like items will be gathered together in the same order as they 
    appeared in the data set before the pass.  
    
    A side-effect is that the data doesn't start to seem ordered until a few passes in.  Like a 
    rubix cube, the data may seem to be thrown out of whack before it starts to come together.  
    In the following example, 41 is the smallest number but doesn't end up as the first item in 
    the data array until after the 3rd pass.

    Ex base-10 counting system (attempted to align text to make it easier to read): 
        - Unsorted:                     12313,  8795, 265328,     41,   1613
        - After 1st pass (1s digit):       41, 12313,   1613,   8795, 265328
        - After 2nd pass (10s digit):   12313,  1613, 265328,     41,   8795
        - After 3rd pass (100s digit):    041, 12313, 265328,   1613,   8795
        - After 4th pass (1000s digit):  0041,  1613,  12313, 265328,   8795
        - After 5th pass (10000s):      00041, 01613,  12313,  08795, 265328
    Notice 2 things:
    (1) After the 1st pass, the two items with a '3' in the 1s digit appeared in the same order 
        that they did before.  Likewise after the 2nd pass, the two items with a '1' in the 10s 
        digit appeared in the same order that they did before.
    (2) Some numbers started having 0s prepended to them when the pass was looking at a digit 
        that didn't exist.  In an integer representation on a computer, that digit DID exist in 
        binary form.  It was 0.  So I added the 0 to make it more clear.  If I didn't and I was 
        looking at the 3rd pass, I would be tempted to think that 12313, which has a 3 (the 
        smallest 100s in this data set) in the 100s digit, should go before 41, which has 
        nothing in the 100s digit.  Wrong.  There IS a value in 41's 100s digit: an implicit 0.  
        So I explicitly added it.

    This is all well and good and really nice to tell people about on paper.  The beauty of this 
    algorithm is that, without relying on the value of adjacent entries in the data set, this 
    algorithm is particularly well suited to a parallel implementation on a GPU.  I will not be 
    covering that right now, so please bear with me.

    ----------------------------------------------------------------------------------------------
    Visualization: 
    https://www.cs.usfca.edu/~galles/visualization/RadixSort.html
    
    PLEASE watch before continuing :).  The following algorithm will seem like a hairy mess of 
    "where is this going?" without an understanding of what it looks like after each pass.  This 
    was very helpful for me.  It works on the 1s digit, then the 10s digit, then the 100s digit, 
    and it could be expanded to operate on however many powers of 10 are needed.

    ----------------------------------------------------------------------------------------------
    Background:

    In math terminology, the "radix" (or "base") is the number of unique digits, including zero, 
    that are used to represent a "positional numeral system".  That is, the numbers are composed 
    of a base set of numbers (ex: 0-9) whose values are dictated by their position in the number 
    (ex: the '9' in 963 has a different value than the '9' in 59).  

    Example of positional numeral system: 
        Binary unsigned integer.  Each increasingly-significant '1' bit adds 2^(bit position) to 
        the number.
    
    Example of non-positional numeral system: 
        Binary signed integer.  The most significant bit denotes positive (0) or negative (1) 
        and does not contribute to its quantity along a number line.  
    
    Example of non-positional numeral system: 
        A float.  The sign bit, mantissa, and exponent are all in the same binary chunk, but the 
        binary values in each of them cannot be looked at individually to determine how they 
        contribute to the value.  While we could look at, say, the 11th bit of an unsigned 
        number and conclude that it adds exactly 2^11 to the number, we cannot look at the 11th 
        bit of a float and conclude exactly how the number's value is affected.

    Radix sorting performance over the bits can be improved by working on two bits at a time 
    (0th and 1st bit, then 2nd and 3rd bit, then 4th and 5th bit, then 6th and 7th bit, etc).  
    Credit for the idea:
    - Code Project article (beware the code; it works, but it has zero comments anywhere and the 
        article glosses over how it works) 
        (https://www.codeproject.com/Articles/543451/Parallel-Radix-Sort-on-the-GPU-using-Cplusplus-AMP).
    - http://www.sci.utah.edu/~csilva/papers/cgf.pdf


    ----------------------------------------------------------------------------------------------
    Algorithm

    Radix sorting takes advantage of the positional property of the numbers' construction.  As 
    previously mentioned, the algorithm performs multiple passes therough the values' base 
    digits or bits.  Each pass performs some counting work to prep for the "desitnation index" 
    calculation for every single entry.
    
    "Sort" step: 
        - Ex: 2nd pass (10s digit for base-10, 1st bit for base-2 (not the 0th bit; bit 
            positions start at 0, which was dealt with for the 1st pass))
        - Base-10 counting system: 
            - Calculated index for entries with 0 in the 10s digit: 
                - All like items (0 in the 10s digit) that came before it.  This was determined 
                in the "scan" step.
            - Calculated index for entries with 1 in the 10s digit: 
                - All items with a 0 in the 10s digit +
                - All like items that came before it
            - Calculated index for entries with 2 in the 10s digit: 
                - All items with a 0 in the 10s digit + 
                - All items with a 1 in the 10s digit + 
                - All like items that came before it
            - Repeat for 3s, 4s, 5s, ..., 9s.
            - Result: 
                - All entries with a 0 in the 10s digit will come first.
                - All entries with a 1 in the 10s digit will come second.
                - All entries with a 2 in the 10s digit will come third.
                - ...
                - All entries with a 9 in the 10s digit will come last.
        - Base-2 counting system: 
            - Calculated index for entries with 0 in the 1st bit: 
                - All like items (0 in the 1st bit) that came before it
            - Calculated index for entries with 1 in the 1st bit: 
                - All items with a 0 in the 1st bit + 
                - All like items that came before it.
            - Result:
                - All entries with a 0 in the 1st bit will come first.
                - All entries with a 1 in the 1st bit will come last.

    ----------------------------------------------------------------------------------------------
    Prep for "how many of each value" counters

    These counters allow the sorting algorithm to determine, for each entry, how many other 
    entries definitely come before it.  For example, in a base-10 counting system on the 5th 
    pass (10,000th digit), all entries with a 0, 1, 2, 3, and 4 in the 10,000th digit definitely 
    come before it.

    By "each value" I mean, for whatever digit (or bit if base-2) is under consideration, how 
    many entries have a 0 in this digit, how many have a 1 in this digit, how many have a 2 in 
    this digit, ..., how many have a 9 in this digit (for base-2, only need to count how many 0s 
    and 1s).  

    The routine is quite simple.  For each entry in the data set:
    - Base-10 counting system: 
        - Needs 10 counters.
        - On the 1st pass, how many 0s in the 1s digit.
        - On the 2nd pass, how many 0s in the 10s digit.  
        - On the 3rd pass, how many 0s in the 100s digit.
        - Repeat for however many digits are in the largest number.
        - Do this also for the 1s, 2s, 3s, 4s, ... 9s.
    - Base-2 counting system: 
        - Needs 2 counters
        - On the 1st pass, how many 0s in the 0th bit.
        - On the 2nd pass, how many 0s in the 1st bit.
        - On the 3rd pass, how many 0s in the 2nd bit.
        - Repeat for however many bits are in this data type (ex: 32bit unsigned int).
        - Do this also for the 1s.
    
    ----------------------------------------------------------------------------------------------
    Prep for "how many like items came before me" counters

    This step in most need of optimization.  A brute force method will require a counter for 
    every item in the data set.  Unfortunately, this is the only option for a GPU implementation 
    because the better approach relies on accessing values in a particular order.  The Code 
    Project example mentioned at the beginning used this approach.  A more efficient algorithm 
    like that given in the "radix sort visualization" link given earlier is much better.  I am 
    explaining both because I have read both.

    MORE EFFICIENT (CPU-ONLY)
    Add together adjacent entries for the "how many of each value" counters (how many 1s in a 
    particular digit, how many 2s, how many 3s, etc.) to get an upper bound index.  Then back 
    fill. 

    Ex base-10 counting system (from visualization link earlier):
    - 30 random values between 0 and 999:
        9 780 143 762 370 579 644 920 516 631 141 892 416 56 384 448 856 213 4 136 522 214 879 379 281 340 330 938 368 835
    - First pass (1s digit)
        - num 0s: 5
        - num 1s: 3
        - num 2s: 3
        - num 3s: 2
        - num 4s: 4
        - num 5s: 1
        - num 6s: 5
        - num 7s: 0
        - num 8s: 3
        - num 9s: 4

    - For each counter, add all the lesser values' counters to your own.  Subtract 1 to get an 
        upper bound index for these like values on this pass.
        - num + prev values 0: 5 + 0 = 5 (0 prior entries)
        - num + prev values 1: 3 + 5 = 8
        - num + prev values 2: 3 + 8 = 11
        - num + prev values 3: 2 + 11 = 13
        - num + prev values 4: 4 + 13 = 17
        - num + prev values 5: 1 + 17 = 18
        - num + prev values 6: 5 + 18 = 23
        - num + prev values 7: 0 + 23 = 23
        - num + prev values 8: 3 + 23 = 26
        - num + prev values 9: 4 + 26 = 30

    - Begin at the end of the array and walk backwards
        - 835: 
            - 1st pass, so this is a 5.  
            - Calculated index = (18 -= 1) = 17.
            - Why 18? See "num + prev values" counters for items with a 5 in the 1s digit.
        - 368: 
            - 1st pass, so this is an 8.  
            - Calculated index = (26 -= 1) = 25.
        - 938: 
            - 1st pass, so this is an 8.  
            - Calculated index = (25 -= 1) = 24.   
            - Notice that the counter was decremented from the first '8'.
        - 330: 
            - 1st pass, so this is a 0.  
            - Calculated index = (5 -= 1) = 4.
        - 340: 
            - 1st pass, so this is a 0.  
            - Calculated index = (4 -= 1) = 3.  
            - Notice that the counter was decremented from the first '0'.
        - 281: 
            - 1st pass, so this is a 1.  
            - Calculated index = (8 -= 1) = 7.
        - Repeat for the rest of the values.

    Unfortunately, while this approach is very nifty, it won't work on the GPU, which cannot 
    "walk" through an array.  It accesses the whole thing almost at once and cannot guarantee 
    that the first thread to decrement a counter for the value at that thread's index will be 
    the last of that item to appear in the array.  Using the example data above, the GPU could 
    not guarantee that the first item to decrement the '5' counter will be 835, likewise it 
    cannot guarantee that the first item to decrement the '0' counter will be 330, or any of the 
    other counters.

    BRUTE FORCE (works on GPU)
    This algorithm will be slower on the CPU, but it can work for the GPU while the previous 
    algorithm can't.  For each entry in the data set, determine how many like items came before 
    it.  Often called the "prefix sums".  Best case scenario is O(1) for the first item in the 
    data set and worsens exponentially as the loop goes on.  This is O(N(N+1)/2), which is a 
    subset of O(N^2).  Put simply, "it ain't good".

    This approach can be made less naive on the GPU by having each thread only scan for prior 
    items within its own work group (or thread "tile" in CUDA).  If the "how many of each value" 
    counters are also computed by work group (that is, for a base-10 counting system, there are 
    10 counters for 0-9 for the first work group, another 10 counters for the second work group, 
    another 10 for the third, etc.), then the destination index can be computed fairly 
    efficiently by adding up the results of all like numbers in previous groups + however many 
    came before the current value.  

    Ex base-10 counting system: 
    - Suppose there are 4 work groups, each with 10 counters. 
    - Suppose that each work group has 256 threads.
    - Suppose that the current thread is looking at item 238 within work group 3, and that this 
        item is the number 824.  
    - If this is the first pass, then we are looking at the 1s digit and therefore want to know 
        to how many items ending in 4 came before it.  
    - Scan through the previous 237 items that this work group is examining.  
    - Suppose we find 11 prior occurances.  
    - Now suppose, on the "how many many of each value" step, 
        - Work group 1 had 23 items ending with a 4 in the 1st digit
        - Work group 2 had 16
        - Work group 3 had 7 
        - Work group 4 had 29
    - This thread is in work group 3, so disregard work group 4, and since I only want to know 
        how many PREVIOUS like items came before in work group 3, then I need the total number 
        of like items and will disregard work group 3's values as well.
    - The calculated index this thread's item is:
        - 23 (number entries with a 4 in the 1s digit in work group 1) + 
        - 16 (number in work group 2) + 
        - 4 (number of "4s" that came before the current item in this thread's work group, which 
            is work group 3).
        - (-1) (convert from "number of items" to index values)
        - 23 + 16 + 4 - 1 = 42
        
        Note: I totally made up these numbers.  It was entirely unintentional that the result 
        was the Meaning of Life.  I don't know what the Ultimate Question is though, so this 
        number, while possibly serendipitous, is unhelpful for our long-term purpose.  It will, 
        however, tell us where 824 should appear in the destination array after the first pass.

    - Overview:
        - Base-10 counting system: 
            - Suppose we are on the 3rd pass (100s digit) and are looking at entries with a '5' 
                in this digit.  
            - If the current entry has a '5' in the 100s digit, then look through all previous 
                entries in the data set and figure out how many entries came before this one 
                that also had a 5 in the 100s digit.  
        - Base-2 counting system: 
            - Suppose we are on the 10th pass (10th bit) and are looking at entries with a '1' 
                in this bit.  
            - If the current entry has a '1' in the 10th bit, then look through all previous 
                entries in the data set and figure out how many entries came before this one 
                that also had a 1 in the 10th bit.
        - Ex: base-10, 5th pass:
        - For each item in the data set
            - If there is a 0 in the 10000s digit
                - For each item in the data set prior to this one, if it also has a 0 in the 
                    10000s digit, increment a "number of like items that came before" counter.
            - else if there is a 1 in the 10000s digit
                - For each item in the data set prior to this one, if it also has a 1 in the 
                    10000s digit, increment a "number of like items that came before" counter.
            - else if there is a 2 in the 10000s digit
                - For each item in the data set prior to this one, if it also has a 2 in the 
                    10000s digit, increment a "number of like items that came before" counter.
            - etc.
    
John Cox, 2/2017
------------------------------------------------------------------------------------------------*/