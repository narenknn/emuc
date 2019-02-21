
class EmucInTrans #(parameter type T);
   task configure(input string p);
      $display(p);
   endtask
endclass // EmucInTrans

typedef struct {
  bit [63:0] _addr;
} REQ;

module sendOutData;

   REQ req;
   int unsigned retVal;
   EmucInTrans #(.T(REQ)) intrans = new;

   initial begin
//      retVal = cGetReq(tStamp, req);
//      if (0 != retVal) break;
      intrans.configure($psprintf("from: %m"));
      $display("finish of sendOutData::initial");
   end


endmodule // sendOutData

module sendOutData2;

   REQ req;
   int unsigned retVal;
   EmucInTrans #(.T(REQ)) intrans = new;

   initial begin
//      retVal = cGetReq(tStamp, req);
//      if (0 != retVal) break;
      intrans.configure($psprintf("from: %m"));
      $display("finish of sendOutData2::initial");
   end

   sendOutData so();

endmodule // sendOutData

module tt;

   import "DPI-C" task initFunc();
   import "DPI-C" task pollOnce();
   sendOutData2 so2();

   bit clk;
   initial forever #5 clk = ~clk;

   initial begin
      initFunc();
      $display("finish of initFunc block");
   end
   initial begin
      repeat(100) begin
         @(posedge clk);
         pollOnce();
      end
      $display("Finish of poll block");
      $finish;
   end

endmodule // tt
