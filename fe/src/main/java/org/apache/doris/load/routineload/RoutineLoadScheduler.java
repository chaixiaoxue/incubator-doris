// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

package org.apache.doris.load.routineload;

import org.apache.doris.catalog.Catalog;
import org.apache.doris.common.LoadException;
import org.apache.doris.common.MetaNotFoundException;
import org.apache.doris.common.util.Daemon;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.util.List;

public class RoutineLoadScheduler extends Daemon {

    private static final Logger LOG = LogManager.getLogger(RoutineLoadScheduler.class);

    private RoutineLoadManager routineLoadManager = Catalog.getInstance().getRoutineLoadManager();

    @Override
    protected void runOneCycle() {
        try {
            process();
        } catch (Throwable e) {
            LOG.error("failed to schedule jobs with error massage {}", e.getMessage(), e);
        }
    }

    private void process() {
        // update
        routineLoadManager.rescheduleRoutineLoadJob();
        // get need schedule routine jobs
        List<RoutineLoadJob> routineLoadJobList = null;
        try {
            routineLoadJobList = getNeedScheduleRoutineJobs();
        } catch (LoadException e) {
            LOG.error("failed to get need schedule routine jobs");
        }

        LOG.debug("there are {} job need schedule", routineLoadJobList.size());
        for (RoutineLoadJob routineLoadJob : routineLoadJobList) {
            try {
                // judge nums of tasks more then max concurrent tasks of cluster
                int currentConcurrentTaskNum = routineLoadJob.calculateCurrentConcurrentTaskNum();
                int totalTaskNum = currentConcurrentTaskNum + routineLoadManager.getSizeOfIdToRoutineLoadTask();
                if (totalTaskNum > routineLoadManager.getTotalMaxConcurrentTaskNum()) {
                    LOG.info("job {} concurrent task num {}, current total task num {}. "
                                    + "desired total task num {} more then total max task num {}, "
                                    + "skip this turn of job scheduler",
                            routineLoadJob.getId(), currentConcurrentTaskNum,
                            routineLoadManager.getSizeOfIdToRoutineLoadTask(),
                            totalTaskNum, routineLoadManager.getTotalMaxConcurrentTaskNum());
                    break;
                }
                // divide job into tasks
                List<RoutineLoadTaskInfo> needScheduleTasksList =
                        routineLoadJob.divideRoutineLoadJob(currentConcurrentTaskNum);
                // save task into queue of needScheduleTasks
                routineLoadManager.getNeedScheduleTasksQueue().addAll(needScheduleTasksList);
            } catch (MetaNotFoundException e) {
                routineLoadJob.updateState(RoutineLoadJob.JobState.CANCELLED);
            }
        }

        LOG.debug("begin to check timeout tasks");
        // check timeout tasks
        List<RoutineLoadTaskInfo> rescheduleTasksList = routineLoadManager.processTimeoutTasks();
        routineLoadManager.getNeedScheduleTasksQueue().addAll(rescheduleTasksList);
    }

    private List<RoutineLoadJob> getNeedScheduleRoutineJobs() throws LoadException {
        return routineLoadManager.getRoutineLoadJobByState(RoutineLoadJob.JobState.NEED_SCHEDULE);
    }


}
