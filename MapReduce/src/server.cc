#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <unistd.h>
#include <grpcpp/grpcpp.h>

#include "utils.h"
#include "mapreduce.grpc.pb.h"

using grpc::Server; // grpc Server class
using grpc::ServerBuilder; // config to build Server class
using grpc::ServerContext; // server context representing single rpc call
using grpc::Status; // Status class for server status response

// ClusterService implementation
class ClusterServiceImpl final : public mapreduce::ClusterService::Service {
public:
    Status InitCluster(ServerContext* context, const mapreduce::InitClusterRequest* request,
                       mapreduce::InitClusterResponse* response) override {
        int32_t cluster_id = SetUid();
        response->set_cluster_id(cluster_id);
        response->set_ready(true);

        std::cout << "[InitCluster]: Received init cluster request: num_mappers: "
                  << request->num_mappers() << " num_reducers: " << request->num_reducers() << std::endl;

        // spawn mappers
        for (int i = 0; i < request->num_mappers(); ++i) {
            pid_t pid = fork(); // fork 
            if (pid == 0) {
                // child (mapper)
                std::cout << "[InitCluster]: Spawned mapper process with PID: " << getpid() << std::endl;
                // Execute mapper process logic here
                // ...
                exit(0); // Exit the child process
            } else if (pid < 0) {
                std::cerr << "[InitCluster]: Failed to spawn mapper process" << std::endl;
            } else {
                // parent
                mapper_pids_.push_back(pid); // add child pid to mapper list
            }
        }

        // spawn reducers
        for (int i = 0; i < request->num_reducers(); ++i) {
            pid_t pid = fork();  // fork
            if (pid == 0) {
                // child (reducer)
                std::cout << "[InitCluster]: Spawned reducer process with PID: " << getpid() << std::endl;
                // Execute reducer process logic here
                // ...
                exit(0); // Exit the child process
            } else if (pid < 0) {
                std::cerr << "[InitCluster]: Failed to spawn reducer process" << std::endl;
            } else {
                // parent
                reducer_pids_.push_back(pid); // add child pid to reducer list
            }
        }

        return Status::OK;
    }

private:
    std::vector<pid_t> mapper_pids_; // list for mapper pids
    std::vector<pid_t> reducer_pids_; // list for reducer pids

};

// JobService implementation
class JobServiceImpl final : public mapreduce::JobService::Service {
public:
    Status SubmitJob(ServerContext* context, const mapreduce::SubmitJobRequest* request,
                     mapreduce::SubmitJobResponse* response) override {
        // Implement the logic for submitting a new MapReduce job
        // You can access the job details from the request object
        // and set the job_id and submitted fields in the response object

        // Example implementation:
        int32_t job_id = SetUid();
        response->set_job_id(job_id);
        response->set_submitted(true);

        std::cout << "[SubmitJob]: Received job request with cluster ID: " << request->cluster_id() << std::endl;
        std::cout << "Input file: " << request->input_file() << std::endl;
        std::cout << "Map function: " << request->map_function() << std::endl;
        std::cout << "Reduce function: " << request->reduce_function() << std::endl;
        std::cout << "Output location: " << request->output_location() << std::endl;

        // Spawn mapper and reducer processes, assign tasks, etc.

        return Status::OK;
    }
};

// run master on localhost:port
void RunServer() {
    std::string server_address("0.0.0.0:50051");
    JobServiceImpl job_service; // init job service 
    ClusterServiceImpl cluster_service; // init cluster service

    ServerBuilder builder; // init server builder
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());  // bind and listen on localhost
    builder.RegisterService(&job_service); // handle job service request
    builder.RegisterService(&cluster_service); // handle cluster service request

    // build the server
    // server instance on heap
    std::unique_ptr<Server> server(builder.BuildAndStart()); 
    std::cout << "Server listening on " << server_address << std::endl;

    // grpc internal thread pool can handle concurrent client requests
    server->Wait(); // blocking
}

int main(int argc, char** argv) {
    RunServer();
    return 0;
}
