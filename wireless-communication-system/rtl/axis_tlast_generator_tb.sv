`timescale 1ns / 1ps

module axis_tlast_generator_tb;
    localparam integer PACKET_BEATS = 8;

    reg clk = 1'b0;
    reg resetn = 1'b0;
    reg [7:0] input_data = 8'h00;
    reg input_valid = 1'b0;
    reg output_ready = 1'b0;
    wire input_ready;
    wire [7:0] output_data;
    wire output_keep;
    wire output_last;
    wire output_valid;

    integer accepted = 0;
    integer last_count = 0;
    integer expected_data = 0;

    always #5 clk = ~clk;

    axis_tlast_generator #(
        .DATA_WIDTH(8),
        .PACKET_BEATS(PACKET_BEATS)
    ) dut (
        .aclk(clk),
        .aresetn(resetn),
        .s_axis_tdata(input_data),
        .s_axis_tvalid(input_valid),
        .s_axis_tready(input_ready),
        .m_axis_tdata(output_data),
        .m_axis_tkeep(output_keep),
        .m_axis_tlast(output_last),
        .m_axis_tvalid(output_valid),
        .m_axis_tready(output_ready)
    );

    always @(posedge clk) begin
        if (resetn && output_valid && output_ready) begin
            accepted = accepted + 1;

            if (output_data !== expected_data[7:0] || output_keep !== 1'b1) begin
                $fatal(1, "TDATA/TKEEP mismatch");
            end

            if (output_last) begin
                last_count = last_count + 1;
                if ((accepted % PACKET_BEATS) != 0) begin
                    $fatal(1, "TLAST occurred on beat %0d", accepted);
                end
            end else if ((accepted % PACKET_BEATS) == 0) begin
                $fatal(1, "TLAST missing on beat %0d", accepted);
            end

            expected_data = expected_data + 1;
        end
    end

    initial begin
        repeat (3) @(posedge clk);
        @(negedge clk);
        resetn = 1'b1;
        input_valid = 1'b1;
        output_ready = 1'b1;

        fork
            begin : drive_data
                integer beat;
                for (beat = 0; beat < (2 * PACKET_BEATS); beat = beat + 1) begin
                    input_data = beat[7:0];
                    @(posedge clk);
                    while (!input_ready) begin
                        @(posedge clk);
                    end
                    @(negedge clk);
                end
                input_valid = 1'b0;
            end

            begin : apply_backpressure
                repeat (6) @(negedge clk);
                output_ready = 1'b0;
                repeat (3) @(negedge clk);
                output_ready = 1'b1;
            end
        join

        @(posedge clk);

        if (last_count != 2) begin
            $fatal(1, "Expected 2 TLAST pulses, observed %0d", last_count);
        end

        $display("axis_tlast_generator_tb PASSED");
        $finish;
    end

endmodule
