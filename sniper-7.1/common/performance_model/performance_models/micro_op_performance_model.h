#ifndef __MICRO_OP_PERFORMANCE_MODEL_H
#define __MICRO_OP_PERFORMANCE_MODEL_H

#include "performance_model.h"
#include "instruction.h"
#include "interval_timer.h"
#include "stats.h"
#include "subsecond_time.h"
#include "dynamic_micro_op.h"

#include <fstream>

#define DEBUG_INSN_LOG 0
#define DEBUG_DYN_INSN_LOG 0
#define DEBUG_CYCLE_COUNT_LOG 0

class CoreModel;
class Allocator;

class MicroOpPerformanceModel : public PerformanceModel
{
public:
   MicroOpPerformanceModel(Core *core, bool issue_memops);
   ~MicroOpPerformanceModel();

protected:
   const CoreModel *m_core_model;

   virtual boost::tuple<uint64_t,uint64_t> simulate(const std::vector<DynamicMicroOp*>& insts) = 0;
   virtual void notifyElapsedTimeUpdate() = 0;
   void doSquashing(std::vector<DynamicMicroOp*> &current_uops, uint32_t first_squashed = 0);

private:
   void handleInstruction(DynamicInstruction *instruction);

   static MicroOp* m_serialize_uop;
   static MicroOp* m_mfence_uop;
   static MicroOp* m_memaccess_uop;

   Allocator *m_allocator; // Per-thread allocator for DynamicMicroOps
   const bool m_issue_memops;

   std::vector<DynamicMicroOp*> m_current_uops;
   // An std::set would sound like a better choice for these, but since the number of elements
   // is usually small (one or two, except for some rare vector instructions) a linear search
   // is fast enough; while std::vector does *much* fewer memory allocations/deallocations
   std::vector<IntPtr> m_cache_lines_read;
   std::vector<IntPtr> m_cache_lines_written;

   UInt64 m_dyninsn_count;
   UInt64 m_dyninsn_cost;
   UInt64 m_dyninsn_zero_count;

#if DEBUG_DYN_INSN_LOG
   FILE *m_dyninsn_log;
#endif
#if DEBUG_INSN_LOG
   FILE *m_insn_log;
#endif
#if DEBUG_CYCLE_COUNT_LOG
   FILE *m_cycle_log;
#endif

   SubsecondTime m_cpiITLBMiss;
   SubsecondTime m_cpiDTLBMiss;
   SubsecondTime m_cpiUnknown;
   SubsecondTime m_cpiMemAccess;

  private:
   // Trace collections.
   struct Micro_Op_Light_Weight
   {
      UInt64 eip;

      enum class Operation : int
      {
         EXE,
         BRANCH,
         LOAD,
         STORE,
         MAX
      }opr;

      UInt64 load_or_store_addr; // L/S address
      UInt64 size;

      bool taken; // For branch instruction

      void setEIP(UInt64 _eip) {eip = _eip;}
      UInt64 getEIP() {return eip;}

      void setLoad() {opr = Operation::LOAD;}
      bool isLoad() {return opr == Operation::LOAD;}

      void setStore() {opr = Operation::STORE;}
      bool isStore() {return opr == Operation::STORE;}

      void setExe() {opr = Operation::EXE;}
      bool isExe() {return opr == Operation::EXE;}

      void setBranch() {opr = Operation::BRANCH;}
      bool isBranch() {return opr == Operation::BRANCH;}

      void setLoadStoreAddr(UInt64 addr) {load_or_store_addr = addr;}
      UInt64 getLoadStoreAddr() {return load_or_store_addr;}

      void setPayloadSize(UInt64 _size) {size = _size;}
      UInt64 getPayloadSize() {return size;}

      void setTaken(bool _taken) {taken = _taken;}
      bool isTaken() {return taken;}
   };
   std::vector<Micro_Op_Light_Weight>lw_micro_ops;

   // The directory that contains all the generated CPU traces.
   std::string cpu_trace_out_dir;

   // Are we in CPU trace gen mode?
   const bool cpu_trace_gen_mode = false;
   // Collect traces every () instructions, default: 250M instructions.
   // const UInt64 fire_duration = 250000000;
   const UInt64 fire_duration = 0;
   // Number of instructions to collect for every firing, default: 10M instructions.
   const UInt64 num_instructions = 250000000;
   // Number of fires, default: 12
   const UInt64 num_fires = 1;

   std::ofstream output;

   bool collecting = false;

   UInt64 total_instructions = 0;
   UInt64 passed_instructions = 0;
   UInt64 collected_instructions = 0;
   UInt64 num_fires_done = 0;

   void CPUTraceGen(DynamicInstruction *dynins);
};

#endif // __MICRO_OP_PERFORMANCE_MODEL_H
