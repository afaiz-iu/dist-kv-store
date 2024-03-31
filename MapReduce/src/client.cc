#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>

#include "mapreduce.grpc.pb.h"

using grpc::Channel; // channel for communicating to grpc server 
using grpc::ClientContext; // client context to be sent with rpc calls from client side
using grpc::Status; // Status class of grpc to hold return status

class MapReduceClient {
public:
    MapReduceClient(std::shared_ptr<Channel> channel)
        : cluster_stub_(mapreduce::ClusterService::NewStub(channel)),   // init cluster service stub
        job_stub_(mapreduce::JobService::NewStub(channel)) {}  // init the job service stub

    // call InitCluster() return cluster_id
    int32_t InitCluster(int32_t num_mappers, int32_t num_reducers) {
        ClientContext context;
        mapreduce::InitClusterRequest request;  // init cluster request message
        request.set_num_mappers(num_mappers); // set num_mappers
        request.set_num_reducers(num_reducers); // set num_reducers

        mapreduce::InitClusterResponse response;  // init cluster response message
        Status status = cluster_stub_->InitCluster(&context, request, &response); // call InitCluster() on master

        if (!status.ok()) {
            std::cerr << "[InitCluster]: FAIL: " << status.error_message() << std::endl;
            return -1;
        }

        std::cout << "[InitCluster]: Ready: " << response.cluster_id() << std::endl;
        return response.cluster_id(); // return cluster_id field of the response msg 
    }

    // call SubmitJob() return job_id
    int32_t SubmitJob(int32_t cluster_id, const std::string& input_file,
                      const std::string& map_function, const std::string& reduce_function,
                      const std::string& output_location) {
        ClientContext context;
        mapreduce::SubmitJobRequest request; // init job request message
        request.set_cluster_id(cluster_id); // set cluster id
        request.set_input_file(input_file); // set input file location 
        request.set_map_function(map_function); // set map function location
        request.set_reduce_function(reduce_function); // set reduce function location
        request.set_output_location(output_location); // set output file location

        mapreduce::SubmitJobResponse response; // job response instance
        Status status = job_stub_->SubmitJob(&context, request, &response);

        if (!status.ok()) {
            std::cerr << "[SubmitJob]: FAIL: " << status.error_message() << std::endl;
            return -1;
        }

        std::cout << "[SubmitJob]: Submit: " << response.job_id() << std::endl;
        return response.job_id(); // return job id for the given job details
    }

private:
    std::unique_ptr<mapreduce::ClusterService::Stub> cluster_stub_; // declare pointer to cluster service stub
    std::unique_ptr<mapreduce::JobService::Stub> job_stub_;  // declare pointer to job service stub
};

int main(int argc, char* argv[]) {
    // create gRPC channel to connect to the server
    std::shared_ptr<Channel> channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());

    // instantiate MapReduceClient for the created channel
    MapReduceClient client(channel);

    // init cluster
    int32_t cluster_id = client.InitCluster(4, 2);
    if (cluster_id == -1) {
        return 1;
    }

    // submit job for the cluster id
    int32_t job_id = client.SubmitJob(cluster_id, "/path/to/input/data", "/path/to/map/function",
                                      "/path/to/reduce/function", "/path/to/output/location");
    if (job_id == -1) {
        return 1;
    }

    // Wait for job completion (not implemented)
    // ...

    return 0;
}
