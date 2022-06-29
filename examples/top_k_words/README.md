## top_k_words

This example leverages a parallel pipeline to perform a very typical processing task:

>Given a text file and a positive integer number K, return the K most frequent words in that text.

The process is modeled as a parallel pipeline with the following steps:

1. Read the input file line by line
2. Break each line into a list of words
3. Place each word into a hash table mapping the word to its frequency
4. Use newly discovered <frequency-word> pairs to maintain a K-top list of words

This means that while step (1) is generating lines (while it's reading the input), subsequent steps are already processing data in their own thread. By the time reading the input is finished, we are pretty much done with the processing.
