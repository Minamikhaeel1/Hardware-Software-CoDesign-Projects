`timescale 1ns / 1ps

/*
 * Adds a TLAST pulse to an AXI4-Stream that has TDATA/TVALID/TREADY only.
 * TLAST is asserted on every PACKET_BEATS-th accepted transfer.
 */
module axis_tlast_generator #(
    parameter integer DATA_WIDTH = 8,
    parameter integer PACKET_BEATS = 128
) (
    (* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME aclk, ASSOCIATED_BUSIF S_AXIS:M_AXIS, ASSOCIATED_RESET aresetn" *)
    (* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 aclk CLK" *)
    input  wire                     aclk,

    (* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME aresetn, POLARITY ACTIVE_LOW" *)
    (* X_INTERFACE_INFO = "xilinx.com:signal:reset:1.0 aresetn RST" *)
    input  wire                     aresetn,

    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 S_AXIS TDATA" *)
    input  wire [DATA_WIDTH-1:0]    s_axis_tdata,
    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 S_AXIS TVALID" *)
    input  wire                     s_axis_tvalid,
    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 S_AXIS TREADY" *)
    output wire                     s_axis_tready,

    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 M_AXIS TDATA" *)
    output wire [DATA_WIDTH-1:0]    m_axis_tdata,
    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 M_AXIS TKEEP" *)
    output wire [(DATA_WIDTH/8)-1:0] m_axis_tkeep,
    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 M_AXIS TLAST" *)
    output wire                     m_axis_tlast,
    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 M_AXIS TVALID" *)
    output wire                     m_axis_tvalid,
    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 M_AXIS TREADY" *)
    input  wire                     m_axis_tready
);

    localparam integer COUNT_WIDTH =
        (PACKET_BEATS <= 1) ? 1 : $clog2(PACKET_BEATS);

    reg [COUNT_WIDTH-1:0] beat_count;

    assign s_axis_tready = m_axis_tready;
    assign m_axis_tdata = s_axis_tdata;
    assign m_axis_tkeep = {(DATA_WIDTH/8){1'b1}};
    assign m_axis_tvalid = s_axis_tvalid;
    assign m_axis_tlast = s_axis_tvalid &&
                          (beat_count == PACKET_BEATS - 1);

    always @(posedge aclk) begin
        if (!aresetn) begin
            beat_count <= {COUNT_WIDTH{1'b0}};
        end else if (s_axis_tvalid && s_axis_tready) begin
            if (beat_count == PACKET_BEATS - 1) begin
                beat_count <= {COUNT_WIDTH{1'b0}};
            end else begin
                beat_count <= beat_count + 1'b1;
            end
        end
    end

endmodule
