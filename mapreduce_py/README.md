initial idea regarding execution flow in my mapreduce system using grpc with python. 

Assumptions: 

1. the local input file exists in a directory layout (eg. data/books/<book_category>/<book.pdf> 
User should be able to define a dir layout for us to walk and get the input data for the map reduce job. But the data is stored in local filesystem
2. all communications are over network. not allowed to pass file pointers to remote workers
3. usage of a tcp persistent k-v storage like memcache. this mecache can be accessed by master/workers. so the master can send set of map keys specifying input data chunks to any endpoint 
4. on submitJob master first loads the input data to this persistent k-v store listening on a given port. We assume that the k-v store is on the same local file system. Hence the memcache process can directly read the user supplied file pointer. loading of the data in the k-v store also involves any partitioning method used 
5. all mappers use this k-v store to store intermediate outputs. reducers access this data as input from this k-v store 

initial idea:

1. on application start the master node spawns and listens for any client request 
2. user submits a initcluster request with num_mappers and num_reducers. if no args for num_mappers/reducers are provided then use default
3. master returns ack with cluster id spawning the required number of forked worker processes
4. user submits a map-reduce job specifying the map, reduce functions, input and output file locations. user supplies the cluster id returned before to specify the cluster on which the map and reduce function needs to run
5. master creates a node object for the job and creates job_id for this job. it then inserts it into a queue and responds to user with the job id for submitted job 
6. master then calls a service to handle nodes in the queue. this service uses stream based partitioning to determine partition size for each mapper worker. It calls service to assign the input chunk to the k-v server and maintains metadata for mappings. we persist this mapping to disk as well(for fault tolerance)   
7. master then calls the mapper service for assigning mappers the set of keys required for processing from the k-v store. the mapper workers each work in parallel with their local chunk(accessed from the k-v store). this service returns to the master the mapping of map_ids for this job 
8. on map task completion the mapper workers notify the master, and send the mapper_id, job_id, cluster_id, and set of keys from k-v store indicating the location of mapper intermediate output to the distributed barrier. This barrier implements the distributed group by key.  master keeps track of 
9. the barrier now uses the hash of mapper output keys to assign the reducer for a given mapper key. it notifies master  for the set of reducers assigned for this job_id, cluster_id 
10. the reducer process execute the task and write to output location. on completion reducer notifies the master with the reducer id, job_id, cluster_id
11. master keeps track of the reducer ids completed and mapper ids completed. the log(user and job level) is updated with these messages as they complete 
12. master notifies the user of job completion. Job completion can be determined by checking the user level log or from console